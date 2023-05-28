#ifndef UI_TAG_EDITOR_HPP_
#define UI_TAG_EDITOR_HPP_

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "tags.hpp"
#include "ui.hpp"

namespace ui::tag_editor {

void Show(bool new_tag, tags::tag_ptr existing_tag, void (*edit_callback)(std::optional<tags::tag_ptr>), void (*delete_callback)(tags::tag_ptr));

void Input();
void Render(bool force);
}  // namespace ui::tag_editor

#endif  // UI_TAG_EDITOR_HPP_
