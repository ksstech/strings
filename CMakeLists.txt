idf_component_register(
	SRCS "x_string_general.c" "x_string_to_values.c"
	INCLUDE_DIRS "."
	REQUIRES statistics
	PRIV_REQUIRES common printfx values
)
