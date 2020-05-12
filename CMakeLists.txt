idf_component_register( SRCS x_string_general.c x_string_to_values.c
						INCLUDE_DIRS . 
						REQUIRES common statistics
						PRIV_REQUIRES common syslog printf hal_esp32 values lwip
						)
