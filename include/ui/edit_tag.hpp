#ifndef UI_EDIT_TAG_HPP_
#define UI_EDIT_TAG_HPP_

#include <set>
#include <string>
#include <vector>

#include "tags.hpp"
#include "ui.hpp"

namespace ui::edit_tag {

void Show(bool new_tag, tags::TagId existing_tag_id, void (*callback)(bool, std::set<tags::TagId>));

void Input();
void Render(bool force);
}  // namespace ui::edit_tag

#endif  // UI_EDIT_TAG_HPP_
