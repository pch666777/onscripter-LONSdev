//this file is LONS's default system.
#ifndef __BUIL_IN_SCRIPT__
#define __BUIL_IN_SCRIPT__

const char *__buil_in_script__ =
	"*lons_textgosub__\n"
	"textbtnwait \%5021\n"
	"if \%5021 == -1 gosub *lons_rmenu__\n"
	"texec\n"
	"return\n"
	"*lons_pretextgosub__\n"
	"return\n"
	"*lons_rmenu__\n"
	"return\n";
#endif // !__BUIL_IN_SCRIPT__
