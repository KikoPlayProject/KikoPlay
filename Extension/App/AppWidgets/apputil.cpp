#include "apputil.h"

namespace Extension
{
static const QHash<QString, AppWidgetOption> optionHash = {
    {"id",                AppWidgetOption::OPTION_ID},
    {"x",                 AppWidgetOption::OPTION_X},
    {"y",                 AppWidgetOption::OPTION_Y},
    {"w",                 AppWidgetOption::OPTION_WIDTH},
    {"h",                 AppWidgetOption::OPTION_HEIGHT},
    {"max_w",             AppWidgetOption::OPTION_MAX_WIDTH},
    {"max_h",             AppWidgetOption::OPTION_MAX_HEIGHT},
    {"min_w",             AppWidgetOption::OPTION_MIN_WIDTH},
    {"min_h",             AppWidgetOption::OPTION_MIN_HEIGHT},
    {"w",                 AppWidgetOption::OPTION_WIDTH},
    {"h",                 AppWidgetOption::OPTION_HEIGHT},
    {"visible",           AppWidgetOption::OPTION_VISIBLE},
    {"enable",            AppWidgetOption::OPTION_ENABLE},
    {"tooltip",           AppWidgetOption::OPTION_TOOLTIP},
    {"title",             AppWidgetOption::OPTION_TITLE},
    {"text",              AppWidgetOption::OPTION_TEXT},
    {"placeholder_text",  AppWidgetOption::OPTION_PLACEHOLDER_TEXT},
    {"scale_content",     AppWidgetOption::OPTION_SCALE_CONTENT},
    {"word_wrap",         AppWidgetOption::OPTION_WORD_WRAP},
    {"open_link",         AppWidgetOption::OPTION_OPEN_LINK},
    {"text_selectable",   AppWidgetOption::OPTION_TEXT_SELECTABLE},
    {"align",             AppWidgetOption::OPTION_ALIGN},
    {"count",             AppWidgetOption::OPTION_COUNT},
    {"col_count",         AppWidgetOption::OPTION_COLUMN_COUNT},
    {"selection_mode",    AppWidgetOption::OPTION_LIST_SELECTION_MODE},
    {"check_state",       AppWidgetOption::OPTION_CHECK_STATE},
    {"current_index",     AppWidgetOption::OPTION_CURRENT_INDEX},
    {"items",             AppWidgetOption::OPTION_ITEMS},
    {"editable",          AppWidgetOption::OPTION_EDITABLE},
    {"max_line",          AppWidgetOption::OPTION_MAX_LINE},
    {"spacing",           AppWidgetOption::OPTION_SPACING},
    {"v_spacing",         AppWidgetOption::OPTION_V_SPACING},
    {"h_spacing",         AppWidgetOption::OPTION_H_SPACING},
    {"row-stretch",       AppWidgetOption::OPTION_ROW_STRETCH},
    {"col-stretch",       AppWidgetOption::OPTION_COL_STRETCH},
    {"content_margin",    AppWidgetOption::OPTION_CONTENT_MARGIN},
    {"h_size_policy",     AppWidgetOption::OPTION_H_SIZE_POLICY},
    {"v_size_policy",     AppWidgetOption::OPTION_V_SIZE_POLICY},
    {"root_decorated",    AppWidgetOption::OPTION_ROOT_DECORATED},
    {"alter_row_color",   AppWidgetOption::OPTION_ALTER_ROW_COLOR},
    {"header_visible",    AppWidgetOption::OPTION_HEADER_VISIBLE},
    {"sortable",          AppWidgetOption::OPTION_SORTABLE},
    {"pinned",            AppWidgetOption::OPTION_PINNED},
    {"checkable",         AppWidgetOption::OPTION_CHECKABLE},
    {"checked",           AppWidgetOption::OPTION_CHECKED},
    {"input_mask",        AppWidgetOption::OPTION_INPUT_MASK},
    {"echo_mode",         AppWidgetOption::OPTION_ECHO_MODE},
    {"disable_h_scroll",  AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR},
    {"disable_v_scroll",  AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR},
    {"h_scroll_visible",  AppWidgetOption::OPTION_H_SCROLLBAR_VISIBLE},
    {"v_scroll_visible",  AppWidgetOption::OPTION_V_SCROLLBAR_VISIBLE},
    {"elide_mode",        AppWidgetOption::OPTION_ELIDEMODE},
    {"min",               AppWidgetOption::OPTION_MIN},
    {"max",               AppWidgetOption::OPTION_MAX},
    {"value",             AppWidgetOption::OPTION_VALUE},
    {"text_visible",      AppWidgetOption::OPTION_TEXT_VISIBLE},
    {"step",              AppWidgetOption::OPTION_SINGLE_STEP},
    {"stack_mode",        AppWidgetOption::OPTION_STACK_MODE},
    {"view_mode",         AppWidgetOption::OPTON_VIEW_MODE},
    {"icon_size",         AppWidgetOption::OPTION_ICON_SIZE},
    {"grid_size",         AppWidgetOption::OPTION_GRID_SIZE},
    {"is_uniform_size",   AppWidgetOption::OPTION_IS_UNIFORM_ITEM_SIZE},
    {"btn_group",         AppWidgetOption::OPTION_BUTTON_GROUP},
    {"show_clear_btn",    AppWidgetOption::OPTION_SHOW_CLEAR_BUTTON},
};

static const QHash<QString, AppWidgetLayoutDependOption> layoutDependOptionHash = {
    {"leading-spacing",          AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_SPACING},
    {"trailing-spacing",         AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_SPACING},
    {"leading-stretch",          AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_STRETCH},
    {"trailing-stretch",         AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_STRETCH},
    {"stretch",                  AppWidgetLayoutDependOption::LAYOUT_DEPEND_STRETCH},
    {"align",                    AppWidgetLayoutDependOption::LAYOUT_DEPEND_ALIGNMENT},
    {"row",                      AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW},
    {"col",                      AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL},
    {"row-span",                 AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_SPAN},
    {"col-span",                 AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_SPAN},
    {"row-stretch",              AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_STRETCH},
    {"col-stretch",              AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_STRETCH},
};

static const QHash<QString, AppEvent> eventHash = {
    {"click",                 AppEvent::EVENT_CLICK},
    {"text_changed",          AppEvent::EVENT_TEXT_CHANGED},
    {"return_pressed",        AppEvent::EVENT_RETURN_PRESSED},
    {"item_click",            AppEvent::EVENT_ITEM_CLICK},
    {"item_double_click",     AppEvent::EVENT_ITEM_DOUBLE_CLICK},
    {"check_state_changed",   AppEvent::EVENT_CHECK_STATE_CHANGED},
    {"current_changed",       AppEvent::EVENT_CURRENT_CHANGED},
    {"link_click",            AppEvent::EVENT_LINK_CLICK},
    {"toggled",               AppEvent::EVENT_TOGGLED},
    {"value_changed",         AppEvent::EVENT_VALUE_CHANGED},
    {"scroll_edge",           AppEvent::EVENT_SCROLL_EDGE},
};

AppWidgetOption getWidgetOptionType(const QString &name)
{
    return optionHash.value(name, AppWidgetOption::OPTION_UNKNOWN);
}

AppWidgetLayoutDependOption getWidgetLayoutDependOptionType(const QString &name)
{
    return layoutDependOptionHash.value(name, AppWidgetLayoutDependOption::LAYOUT_DEPEND_UNKNOWN);
}

AppEvent getWidgetEventType(const QString &name)
{
    return eventHash.value(name, AppEvent::EVENT_UNKNOWN);
}

}
