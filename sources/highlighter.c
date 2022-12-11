#include <fites/highlighter.h>
#include <ctype.h>
#include <lang.h>

void highlighter_select_syntax_highlight() {
	state_w()->current_syntax = NULL;
	if (state_r().filename == NULL) return;

	char* ext = strrchr(state_r().filename, '.');

	for (unsigned int j = 0; j < LANGUAGE_ENTRIES; j++) {
		struct syntax* s = &languages[j];
		unsigned int i = 0;
		while (s->filematch[i]) {
			int is_ext = (s->filematch[i][0] == '.');
			if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
				(!is_ext && strstr(state_r().filename, s->filematch[i]))) {
					state_w()->current_syntax = s;
					
					int row;
					for (row = 0; row < state_r().text_row_count; row++) {
						highlighter_update_syntax(&state_w()->text[row]);
					}
					
					return;
				}
			i++;
		}
	}
}

int highlighter_to_color(enum highlighter highlighter) {
	switch (highlighter) {
		case HL_NUMBER: return COLOR_NUMBER;
		case HL_NORMAL: return COLOR_NORMAL;
		case HL_MATCH: return COLOR_MATCH;
		case HL_STRING: return COLOR_STRING;
		case HL_COMMENT: return COLOR_COMMENT;
		default: return COLOR_NORMAL;
	}
	return -1;
}

int is_separator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()#-/*=~%<>[];_", c) != NULL;
}

void highlighter_update_syntax(struct text_row* row) {
	row->highlight = realloc(row->highlight, row->rendered_size);
	memset(row->highlight, HL_NORMAL, row->rendered_size);

	if (state_r().current_syntax == NULL) return;

	char* comment = state_r().current_syntax->line_comment;
	int comment_length = comment ? strlen(comment) : 0;


	int last_separator = 1;
	int in_string = 0;
	int i = 0;
	while (i < row->rendered_size) {
		char this = row->rendered[i];
		unsigned char last_highlight = (i > 0) ? row->highlight[i - 1] : HL_NORMAL;
		int flags = state_r().current_syntax->flags;

		if (comment_length && !in_string) {
			if (!strncmp(&row->rendered[i], comment, comment_length)) {
				memset(&row->highlight[i], HL_COMMENT, row->rendered_size - i);
				break;
			}
		}

		if (flags & HL_HIGHLIGHT_STRINGS) {
			if (in_string) {
				row->highlight[i] = HL_STRING;

				if (this == '\\' && i + 1 < row->rendered_size) {
					row->highlight[i + 1] = HL_STRING;
					i += 2;
					continue;
				}

				if (this == in_string) in_string = 0;
				i++;
				last_separator = 1;
				continue;
			} else {
				if ( this == '"' || this == '\'') {
					in_string = this;
					row->highlight[i] = HL_STRING;
					i++;
					continue;
				}
			}
		}


		if (
			flags & HL_HIGHLIGHT_NUMBERS &&
			isdigit(this) && 
			((
				last_separator ||
				last_highlight == HL_NUMBER
			) ||
			(
				(this == '.' || (this == '_' && HL_HIGHLIGHT_UNDERSCORED_NUMBERS)) &&
				last_highlight == HL_NUMBER
			))) {

			row->highlight[i] = HL_NUMBER;
			i++;
			last_separator = 0;
			continue;
		}
		last_separator = is_separator(this);
		i++;
	}
}