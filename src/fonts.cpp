#include "include/S1D13781/fonts.h"

#define XX(name)                                                                                                       \
	IMPORT_FSTR_LOCAL(name##IndexData, COMPONENT_PATH "/fonts/" #name ".pfi")                                          \
	IMPORT_FSTR_LOCAL(name##ImageData, COMPONENT_PATH "/fonts/" #name ".pbm")
FONT_LIST(XX)
#undef XX

#define XX(name) {&name##IndexData, &name##ImageData},
#define YY(name, width, height) {nullptr, fontData_##name, #name, width, height},
DEFINE_FSTR_ARRAY(fontTable, FontDef, FONT_LIST(XX) LINUX_FONT_LIST(YY));
#undef XX
#undef YY
