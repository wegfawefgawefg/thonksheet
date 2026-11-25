// The views: the sheet, the raster, the recurrence square and the marks for the picked
// neuron. All of them read the raw spike words of the tape from one storage buffer, so a
// frame costs the new tape rows, whatever the sheet's size.

#include "layout.hpp"
#include "renderer/overlay.hpp"
#include "renderer/vulkan_internal.hpp"
#include "view_frame.hpp"

#include <algorithm>
#include <cstring>

namespace vk {

// the kinds (fixed at birth) into their buffer when the shape is new; every slot's tape
// starts over
static bool new_shape(Context& g, const ViewFrame& f) {
    const Tape& t = f.tape;
    if (g.tile_count == t.row_words && g.tape_len == t.len) {
        return true;
    }
    if (!wait(g) || !buffer(g, g.kinds, sizeof(uint64_t) * 3 * std::max(1, f.tile_count),
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        return false;
    }
    uint64_t* kinds = static_cast<uint64_t*>(g.kinds.mapped);
    for (int i = 0; i < f.tile_count; ++i) {
        kinds[i * 3 + 0] = f.inhibitory[i];
        kinds[i * 3 + 1] = f.sensor[i];
        kinds[i * 3 + 2] = f.motor[i];
    }
    for (Slot& s : g.slots) {
        s.uploaded = 0;
    }
    g.tile_count = t.row_words;
    g.tape_len = t.len;
    return true;
}

void upload_views(Context& g, const ViewFrame& f, const ViewSettings& v) {
    const Tape& t = f.tape;
    if (t.len == 0 || !new_shape(g, f)) {
        return;
    }
    Slot& s = now(g);
    const VkDeviceSize tape_size = sizeof(uint64_t) * t.len * t.row_words;
    if (!buffer(g, s.tape, tape_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ||
        !buffer(g, s.marks, sizeof(Mark) * MARKS_MAX, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        return;
    }

    // the tape rows this slot does not have yet
    if (s.uploaded > t.ticks) {
        s.uploaded = 0;
    }
    const uint64_t oldest = t.ticks > static_cast<uint64_t>(t.len) ? t.ticks - t.len : 0;
    uint64_t* dst = static_cast<uint64_t*>(s.tape.mapped);
    for (uint64_t tick = std::max(s.uploaded, oldest); tick < t.ticks; ++tick) {
        const size_t row = tick % static_cast<uint64_t>(t.len);
        std::memcpy(dst + row * t.row_words, &t.words[row * t.row_words],
                    sizeof(uint64_t) * t.row_words);
    }
    s.uploaded = t.ticks;

    // the marks
    build_marks(g.mark_list, f, v);
    s.mark_count = std::min(static_cast<int>(g.mark_list.size()), MARKS_MAX);
    if (s.mark_count > 0) {
        std::memcpy(s.marks.mapped, g.mark_list.data(), sizeof(Mark) * s.mark_count);
    }

    // the per-cell colours, when there are any
    s.overlay_on = build_overlay(g.overlay_cells, f, v);
    const size_t cells = static_cast<size_t>(f.tiles_w) * 8 * f.tiles_h * 8;
    if (!buffer(g, s.overlay, sizeof(uint32_t) * std::max<size_t>(1, cells),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        s.overlay_on = false;
        return;
    }
    if (s.overlay_on && g.overlay_cells.size() == cells) {
        std::memcpy(s.overlay.mapped, g.overlay_cells.data(), sizeof(uint32_t) * cells);
    } else {
        s.overlay_on = false;
    }

    // this slot's set onto its buffers (the slot is idle: begin() waited for it)
    VkDescriptorBufferInfo infos[4] = {
        {s.tape.buffer, 0, VK_WHOLE_SIZE},
        {g.kinds.buffer, 0, VK_WHOLE_SIZE},
        {s.marks.buffer, 0, VK_WHOLE_SIZE},
        {s.overlay.buffer, 0, VK_WHOLE_SIZE},
    };
    VkWriteDescriptorSet writes[4];
    for (int i = 0; i < 4; ++i) {
        writes[i] = VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = s.set;
        writes[i].dstBinding = static_cast<uint32_t>(i);
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &infos[i];
    }
    vkUpdateDescriptorSets(g.device, 4, writes, 0, nullptr);
}

static ViewPush base_push(const Context& g, const ViewFrame& f, const Rect& r) {
    ViewPush p{};
    p.rect[0] = r.x;
    p.rect[1] = r.y;
    p.rect[2] = r.w;
    p.rect[3] = r.h;
    p.extent[0] = static_cast<float>(g.screen.w);
    p.extent[1] = static_cast<float>(g.screen.h);
    const Tape& t = f.tape;
    p.cells_w = static_cast<uint32_t>(f.tiles_w * 8);
    p.cells_h = static_cast<uint32_t>(f.tiles_h * 8);
    p.tiles_w = static_cast<uint32_t>(f.tiles_w);
    p.tile_count = static_cast<uint32_t>(t.row_words);
    p.tape_len = static_cast<uint32_t>(t.len);
    p.newest_row = t.ticks > 0 ? static_cast<uint32_t>((t.ticks - 1) % t.len) : 0;
    p.rows_valid = static_cast<uint32_t>(std::min<uint64_t>(t.ticks, t.len));
    p.sector_tiles = static_cast<uint32_t>(std::max(1, f.sector_tiles));
    p.selected = f.selected.neuron;
    p.word_step = 1;
    return p;
}

// one pipeline's quads over a rect, clipped to it
static void quad(Context& g, VkPipeline pipeline, const ViewPush& p, const Rect& r,
                 uint32_t instances) {
    const int x = std::max(0, static_cast<int>(r.x));
    const int y = std::max(0, static_cast<int>(r.y));
    const int w = std::min(static_cast<int>(r.w), g.screen.w - x);
    const int h = std::min(static_cast<int>(r.h), g.screen.h - y);
    if (w <= 0 || h <= 0) {
        return;
    }
    VkRect2D scissor{{x, y}, {static_cast<uint32_t>(w), static_cast<uint32_t>(h)}};
    vkCmdSetScissor(g.command, 0, 1, &scissor);
    vkCmdBindPipeline(g.command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdPushConstants(g.command, g.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(ViewPush), &p);
    vkCmdDraw(g.command, 6, instances, 0, 0);
}

void record_views(Context& g, const ViewFrame& f, const Layout& l, const ViewSettings& v) {
    const Slot& s = now(g);
    if (f.tape.len == 0 || f.tape.ticks == 0 || g.tile_count != f.tape.row_words ||
        s.tape.buffer == VK_NULL_HANDLE) {
        return;
    }
    VkViewport viewport{0, 0, static_cast<float>(g.screen.w), static_cast<float>(g.screen.h), 0, 1};
    vkCmdSetViewport(g.command, 0, 1, &viewport);
    vkCmdBindDescriptorSets(g.command, VK_PIPELINE_BIND_POINT_GRAPHICS, g.pipeline_layout, 0, 1,
                            &s.set, 0, nullptr);

    // the sheet, then the picked neuron's marks over it
    ViewPush sheet = base_push(g, f, l.sheet);
    const SheetPlace place = sheet_place(l.sheet, f.tiles_w * 8, f.tiles_h * 8, v);
    sheet.origin[0] = place.origin_x;
    sheet.origin[1] = place.origin_y;
    sheet.cell_px = place.cell_px;
    sheet.fade = static_cast<float>(std::max(1, v.fade));
    // lines only where they leave room for the cells: a tile 24 pixels or more across
    const float tile_px = place.cell_px * 8.0f;
    const bool tiles_fit = tile_px >= 24.0f;
    const bool sectors_fit = tile_px * std::max(1, f.sector_tiles) >= 24.0f;
    sheet.flags = (v.tile_lines && tiles_fit ? 1u : 0u) |
                  (v.sector_lines && sectors_fit ? 2u : 0u) | (s.overlay_on ? 4u : 0u);
    quad(g, g.sheet, sheet, l.sheet, 1);
    if (s.mark_count > 0) {
        quad(g, g.marks, sheet, l.sheet, static_cast<uint32_t>(s.mark_count));
    }

    // the raster
    ViewPush raster = base_push(g, f, l.raster);
    raster_rows(l.raster, f.tape.row_words, v, &raster.origin[0], &raster.origin[1]);
    raster.mode = static_cast<uint32_t>(v.raster_rows);
    // the rate regulation aims for, so a dying blob looks dark and a saturated one white
    float target = 0;
    for (const SectorView& sector : f.sectors) {
        target += sector.target;
    }
    raster.fade = target / static_cast<float>(std::max(1, f.neuron_count));
    if (f.sectors.empty()) {
        raster.fade = f.mean_rate; // an engine without sectors: against its own mean rate
    }
    quad(g, g.raster, raster, l.raster, 1);

    // the recurrence square, comparing at most 256 words per pair of ticks
    if (l.recur.w > 16) {
        ViewPush recur = base_push(g, f, l.recur);
        recur.mode = std::min(recur.rows_valid, static_cast<uint32_t>(std::max(8, v.recur_ticks)));
        recur.word_step = std::max(1u, recur.tile_count / 256u);
        float usual = 0;
        for (int lag = 2; lag <= LAG_MAX; ++lag) {
            usual += f.lag_similarity[lag - 1];
        }
        recur.fade = usual / (LAG_MAX - 1);
        quad(g, g.recur, recur, l.recur, 1);
    }
}

} // namespace vk
