/* fot: wrap text from (fortune cookie) files */

#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifndef __STDC_ISO_10646__
/* I feel like I should do something with this. In any event, if this isn't
   defined there's no guarantee that `wchar_t` is UCS-4. For example, on
   Windows, it's UCS-2, which means it can only hold BMP characters.
   I would put in a #warning but that's not ANSI C. */
#endif

/* Read a line from `fp` into the buffer `buf`, reallocating the buffer
   and updating the buffer size variable `buf_size` with its new size (in
   characters, not bytes!) as necessary. */
unsigned long int line_from(FILE* fp, wchar_t** buf, unsigned long int* buf_size)
{
	wint_t c;
	unsigned long int chars_read = 0;

	if (*buf == NULL) {
		if ((*buf = malloc(*buf_size * sizeof(wchar_t))) == NULL) {
			fwprintf(stderr, L"Can't allocate %lu bytes for text buffer.\n",
			         *buf_size * sizeof(wchar_t));
			abort();
		}
	}

	while (true) {
		if ((c = getwc(fp)) == WEOF) {
			if (feof(fp) || ferror(fp)) {
				break;
			}
		} else if (c == L'\n') {
			break;
		}
		if (chars_read >= *buf_size) {
			*buf_size *= 2;
			if ((*buf = realloc(*buf, *buf_size * sizeof(wchar_t))) == NULL) {
				fwprintf(stderr,
				         L"Can't allocate %lu bytes for text buffer.\n",
				         *buf_size * sizeof(wchar_t));
				abort();
			}
		}
		(*buf)[chars_read] = c;
		chars_read++;
	}

	(*buf)[chars_read] = '\0';

	/* Try to read an extra character from `fp` to check for an imminent
	   EOF. If feof & ferror both return false, more characters are to come,
	   so push the newly read character back into the stream for next time.
	   Otherwise something's gone wrong or EOF's been reached; the calling
	   function can now check for an error or EOF itself. */
	c = getwc(fp);
	if ((!feof(fp)) && !ferror(fp)) {
		ungetwc(c, fp);
	}

	return chars_read;
}

/* Find where the left margin ends in the single-line string `line`.
   Return a pointer to the first non-margin character in `line`. */
wchar_t* end_of_margin(const wchar_t* line, unsigned int* len)
{
	wchar_t* p;

	*len = 0;

	for (p = (wchar_t*) line; *p; p++) {
		if ((*p == L' ') || (*p == L'\t')) {
			(*len)++;
		} else {
			break;
		}
	}

	return p;
}

/* Write a left margin to standard output. */
void output_margin(const wchar_t* start_of_line, const wchar_t* margin_end, unsigned int* out_len)
{
	wchar_t *p;

	for (p = (wchar_t*) start_of_line; p < margin_end; p++) {
		putwc(*p, stdout);
		(*out_len)++;
		if (*p == L'\t') {
			if (*out_len % 8) {
				(*out_len) += 8 - (*out_len % 8);
			}
		}
	}
}

/* Process the contents of an open file. */
unsigned int process_stream(FILE* fp, unsigned int max_line_len)
{
	wchar_t* line_buf = NULL;
	unsigned long int line_buf_size = 80;  /* not its actual size yet! */
	unsigned line_no = 0;  /* track current line number for error messages */
	unsigned int out_len = 0;
	bool prev_line_read_forced_break = false;
	wchar_t* prev_marg = NULL;
	unsigned int prev_marg_buf_size = 0;
	unsigned int prev_marg_len = 0;
	wchar_t* marg_end;
	bool marg_just_written;
	unsigned int marg_len;
	wchar_t* p;
	wchar_t* q;
	wchar_t* word_start;

	while ((!feof(fp)) && !ferror(fp)) {

		/* Read in a line. */
		line_no++;
		line_from(fp, &line_buf, &line_buf_size);

		/* If this line is empty just emit two newlines
		   and proceed to the next input line. */
		if (line_buf[0] == L'\0') {
			putwc(L'\n', stdout);
			if (!prev_line_read_forced_break) {
				putwc(L'\n', stdout);
			}
			out_len = 0;
			if (prev_marg != NULL) {
				prev_marg_len = 0;
				prev_marg[0] = '\0';
			}
			prev_line_read_forced_break = true;
			continue;
		}

		/* If this line is just a percentage sign (i.e. a fortune cookie
		   delimiter) on its own then output it on its own line. */
		if ((line_buf[0] == L'%') && (line_buf[1] == L'\0')) {
			if (out_len) {
				putwc(L'\n', stdout);
			}
			putwc(L'%', stdout);
			putwc(L'\n', stdout);
			out_len = 0;
			if (prev_marg != NULL) {
				prev_marg_len = 0;
				prev_marg[0] = '\0';
			}
			prev_line_read_forced_break = true;
			continue;
		}

		/* Find where the line's left margin ends and point `p` there. */
		marg_end = end_of_margin(line_buf, &marg_len);
		p = marg_end;
		marg_just_written = false;

		/* Resize the buffer for the previous line's left margin if
		   necessary, and write a left margin if one's needed. */
		if (prev_marg != NULL) {
			if ((marg_len != prev_marg_len) || (marg_len && wmemcmp(line_buf, prev_marg, marg_len))) {
				if (marg_len > prev_marg_buf_size) {
					prev_marg_buf_size = marg_len;
					prev_marg = realloc(prev_marg, marg_len * sizeof(wchar_t));
					if (prev_marg == NULL) {
						fwprintf(stderr, L"Can't reallocate %u bytes for "
						         "left margin buffer.\n",
						         marg_len * sizeof(wchar_t));
						break;
					}
				}
				if (!prev_line_read_forced_break) {
					/* Begin a new line of output. */ 
					putwc(L'\n', stdout);
					out_len = 0;
				}
				/* Output the left margin for the new line of output. */
				output_margin(line_buf, marg_end, &out_len);
				marg_just_written = true;
			} else if (*p == '\0') {
				/* This line is nothing but margin, and that margin is the
				   same as the previous line's. Write the entirety of this
				   line (i.e. its margin followed by a newline). */
				if (out_len) {
					putwc(L'\n', stdout);
				}
				output_margin(line_buf, marg_end, &out_len);
				putwc(L'\n', stdout);
				out_len = 0;
				if (prev_marg != NULL) {
					prev_marg_len = 0;
					prev_marg[0] = '\0';
				}
				prev_line_read_forced_break = true;
				continue;
			}
		} else {
			/* This is the first line of output. Output the left margin. */
			output_margin(line_buf, marg_end, &out_len);
			marg_just_written = true;
		}

		/* Now, at this point this program might be in the middle of a
		   line of output. So check that the first word in the current
		   input line would fit into the current output line. */
		if (out_len && !marg_just_written) {
			q = p;
			while ((*q) && !iswspace(*q)) {
				q++;
			}
			if ((out_len + q - p + 1) > max_line_len) {
				putwc(L'\n', stdout);
				out_len = 0;
				output_margin(line_buf, marg_end, &out_len);
				marg_just_written = true;
			} else {
				putwc(L' ', stdout);
				out_len++;
			}
		}

		/* Iterate over the rest of this input line, trying to write it
		   word by word. */
		while (*p) {

			/* Next, output the word currently pointed to (where "word"
			   here means a series of non-whitespace characters). */
			while ((*p) && !iswspace(*p)) {
				putwc(*p, stdout);
				p++;
				out_len++;
			}

			/* Skip over the coming series of whitespace to the next
			   word, find its length, and determine whether it can fit
			   into the current line of output. If so, output a space,
			   point `p` at that next word and continue the loop. */
			q = p;
			while ((*q) && iswspace(*q)) {
				q++;
			}
			word_start = q;
			while ((*q) && !iswspace(*q)) {
				q++;
			}
			if (!*word_start) {
				/* There are no more words on this input line, move on. */
				break;
			}
			if ((out_len + q - word_start + 1) <= max_line_len) {
				putwc(L' ', stdout);
				out_len++;
				p = word_start;
				continue;
			}

			/* If execution reached this point, the word won't fit.
			   Start a new line (and write the left margin to it). */
			p = word_start;
			putwc(L'\n', stdout);
			out_len = 0;
			output_margin(line_buf, marg_end, &out_len);

		}

		/* The program's now about to move on to the next line, so copy
		   the current line's left margin into the buffer for the previous
		   line's margin, ready for the next line of input. */
		if (prev_marg == NULL) {
			prev_marg_buf_size = marg_len;
			prev_marg = malloc(marg_len * sizeof(wchar_t));
			if (prev_marg == NULL) {
				fwprintf(stderr,
				         L"Can't allocate %u bytes for left margin buffer.\n",
				         marg_len * sizeof(wchar_t));
				break;
			}
		}
		wmemcpy(prev_marg, line_buf, marg_len);
		prev_marg_len = marg_len;

		prev_line_read_forced_break = false;  /* reset for next line */
	}

	free(line_buf);  /* `line_from` allocated a buffer; free it */

	if (out_len) {
		putwc(L'\n', stdout);  /* lines should end in a newline */
	}

	return line_no;  /* for potential error messages in `main` */
}

int main(int argc, char* argv[])
{
	int arg;
	FILE* fp;
	unsigned int line_no;
	unsigned int max_line_len = 78;

	/* Gotta set the locale so wide characters work as advertised. */
	if (setlocale(LC_ALL, "") == NULL) {
		fputws(L"Couldn't set locale from environment variables.\n", stderr);
	}

	/* Interpret options. */
	for (arg = 1; arg < argc; arg++) {
		if (argv[arg][0] != '-') {
			/* The user didn't pass any more options, so don't bother
			   looking for any. */
			break;
		}
		if (argv[arg][2] == '\0') {
			if (argv[arg][1] == '-') {
				/* `argv[arg]` is "--", i.e. the user wants this program to
				   treat all subsequent arguments as filenames. */
				arg++;
				break;
			} else if (argv[arg][1] == 'w') {
				/* `argv[arg]` is "-w", i.e. the user's setting the desired
				   maximum width. */
				arg++;
				if (argc <= arg) {
					fputws(L"The \"-w\" option needs an integer argument.\n",
					       stderr);
				} else {
					errno = 0;
					max_line_len = strtoul(argv[arg], NULL, 10);
					if (errno) {
						fwprintf(stderr,
						         L"Error reading maximum line length: %s.\n",
						         strerror(errno));
						return EXIT_FAILURE;
					}
				}
			} else {
				fwprintf(stderr, L"Unknown option \"%s\". "
				         "Valid options are \"--\" and \"-w LENGTH\".\n",
				         argv[arg]);
				return EXIT_FAILURE;
			}
		} else {
			fwprintf(stderr, L"Unknown option \"%s\". "
			         "Valid options are \"--\" and \"-w LENGTH\".\n",
			         argv[arg]);
			return EXIT_FAILURE;
		}
	}

	/* If no non-option arguments are given, process standard input. */
	if (argc == arg) {
		line_no = process_stream(stdin, max_line_len);
		if (ferror(stdin)) {
			fwprintf(stderr, L"%u: error: %s.\n", line_no, strerror(errno));
		}
	}

	/* Iterate over paths given as arguments, processing each in turn. */
	for (; arg < argc; arg++) {
		if ((fp = fopen(argv[arg], "rb")) == NULL) {
			fwprintf(stderr, L"Can't open %s.\n", argv[arg]);
			continue;
		}
		line_no = process_stream(fp, max_line_len);
		if (ferror(fp)) {
			fwprintf(stderr, L"%s:%u: error: %s.\n",
			         argv[arg], line_no, strerror(errno));
		}
		if (fclose(fp)) {
			fwprintf(stderr, L"Can't close %s.\n", argv[arg]);
		}
	}

	return EXIT_SUCCESS;
}
