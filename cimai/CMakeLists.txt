add_library (cimai 
	"src/misc/thirdparty_impl.c"
	"src/meta_parser.c"
	"src/chart_parser.c"
	"src/parser.c"
)
target_include_directories(cimai
    PUBLIC "include"
)

# TODO: 如有需要，请添加测试并安装目标。
