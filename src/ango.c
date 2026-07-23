#include "ango.h"

int main(int argc, char *argv[])
{
	if (argc == 1)
		usage();
	int c = 0;
	khint_t k;
	arg_t arg;
	memset(&arg, 0, sizeof(arg_t));
	ketopt_t opt = KETOPT_INIT;
	while ((c = ketopt(&opt, argc, argv, 1, "i:o:g:e:vh", long_options)) >= 0)
	{
		if (c == 'h') usage();
		else if (c == 'v')
		{
			puts(VERSION);
			exit(EXIT_SUCCESS);
		}
		else if (c == 'i') arg.in = opt.arg;
		else if (c == 'o') arg.out = opt.arg;
		else if (c == 'g') arg.go = opt.arg;
		else if (c == 'e') arg.egg = opt.arg;
		else if (c == '?') error("Unknown option -%c", opt.opt ? opt.opt : ':');
		else if (c == ':') error("Missing option argument: -%c", opt.opt ? opt.opt : ':');
		else error("Unknown option encountered");
	}
	if (access(arg.in, R_OK)) error("Can't access file [%s]", arg.in);
	if (access(arg.go, R_OK)) error("Can't access file [%s]", arg.go);
	if (access(arg.egg, R_OK)) error("Can't access file [%s]", arg.egg);
	go_t *g = go_init();
	sg_t *s = sg_init();
	let_go(arg.go, g);
	eat_egg(arg.egg, s);
	map_go(arg.in, g, s, arg.out);
	// clean up
	kh_foreach(s, k)
		free((uint32_t *)kh_val(s, k));
	sg_destroy(s);
	kh_foreach(g, k)
	{
		gb_t *gb = kh_val(g, k);
		free(gb->name);
		free(gb->namespace);
		free(gb);
	}
	go_destroy(g);
}

void let_go(const char *fn, go_t *h)
{
	/* [Term]
	 * id: GO:0000017
	 * name: alpha-glucoside transport
	 * namespace: biological_process
	 * */
	khint_t k;
	int absent;
	gb_t *gb = NULL;
	bool term = false;
	kstring_t ks = {0, 0, 0};
	FILE *fp = fopen(fn, "r");
	if (!fp)
		error("Failed to open file [%s] for reading\n", fn);
	while (kgetline(&ks, (kgets_func *)fgets, fp) == 0)
	{
		if (ks.l == 0)
			continue;
		if (strncmp(ks.s, "id: GO:", 7) == 0)
		{
			term = true;
			k = go_put(h, atoi(ks.s + 7), &absent);
			gb = kh_val(h, k) = calloc(1, sizeof(gb));
			goto next;
		}
		if (strncmp(ks.s, "name: ", 6) == 0)
		{
			if (term) gb->name = strdup(ks.s + 6);
			goto next;
		}
		if (strncmp(ks.s, "namespace: ", 11) == 0)
		{
			if (term) gb->namespace = strdup(ks.s + 11);
			term = false;
			goto next;
		}
next:
		ks.l = 0;
	}
	if (ks.m) free(ks.s);
	fclose(fp);
}

void eat_egg(const char *fn, sg_t *h)
{
	/* 0  UNK.EB99@1648030|U-6
	 * 1  UNK.EB99
	 * 2  4564
	 * 3  3
	 * 4  2
	 * 5  4572.M8A7B3,4565.A0A3B6IUU0,4565.A0A3B6HP43
	 * 6  empty or not
	 * 7  empty or not
	 * 8  GO:0006351|100.00;GO:0065003|66.67
	 * */
	khint_t k;
	int i = 0, j = 0;
	kstring_t ks = {0, 0, 0}, ks_sq = {0, 0, 0}, ks_go = {0, 0, 0};
	int *col = NULL, mcol = 0, absent = 0;
	int *col_sq = NULL, mcol_sq = 0, ncol_sq;
	int *col_go = NULL, mcol_go = 0, ncol_go;
	gzFile fp = gzopen(fn, "r");
	if (!fp)
	{
		fprintf(stderr, "Error openning file [%s]\n", fn);
		exit(EXIT_FAILURE);
	}
	while (kgetline(&ks, (kgets_func *)kzgets, fp) == 0)
	{
		ksplit_core(ks.s, '\t', &mcol, &col);
		if (strncmp(ks.s + col[8], "GO:", 3) == 0)
		{
			kputs(ks.s + col[5], &ks_sq);
			ncol_sq = ksplit_core(ks_sq.s, ',', &mcol_sq, &col_sq);
			kputs(ks.s + col[8], &ks_go);
			ncol_go = ksplit_core(ks_go.s, ';', &mcol_go, &col_go);
			for (i = 0; i < ncol_sq; ++i)
			{
				k = sg_put(h, strdup(ks_sq.s + col_sq[i]), &absent);
				uint32_t *gos = kh_val(h, k) = calloc(ncol_go + 1, sizeof(uint32_t));
				for (j = 0; j < ncol_go; ++j)
					gos[j] = atoi(ks_go.s + col_go[j] + 3);
			}
		}
		ks.l = ks_sq.l = ks_go.l = 0;
	}
	if (col) free(col);
	if (col_sq) free(col_sq);
	if (col_go) free(col_go);
	if (ks.m) free(ks.s);
	if (ks_sq.m) free(ks_sq.s);
	if (ks_go.m) free(ks_go.s);
	gzclose(fp);
}

void map_go(const char *in, const go_t *g, const sg_t *s, const char *out)
{
	int i = 0;
	khint_t j, k;
	kstring_t ks = {0, 0, 0};
	FILE *fp = fopen(in, "r"), *fo = out ? fopen(out, "w") : stdout;
	if (!fp)
		error("Failed to open file [%s] for reading\n", in);
	while (kgetline(&ks, (kgets_func *)fgets, fp) == 0)
	{
		fprintf(fo, "%s\t", ks.s);
		if ((k = sg_get(s, ks.s)) != kh_end(s))
		{
			i = 0;
			uint32_t *gos = kh_val(s, k);
			while (gos[i])
			{
				if ((j = go_get(g, gos[i])) != kh_end(g))
				{
					gb_t *gb = kh_val(g, j);
					fprintf(fo, "GO:%07d:%s:%s;", kh_key(g, j), gb->namespace, gb->name);
				}
				++i;
			}
		}
		fputc('\n', fo);
		ks.l = 0;
	}
	if (ks.m) free(ks.s);
	fclose(fp);
}

void horiz(int n, bool bold)
{
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	n = (w.ws_col >= n) ? n : w.ws_col;
	printf("\e[%s90m", bold ? "1;" : "");
	while (n--) fputs("\xe2\x94\x80", stdout);
	printf("\e[%s0m\n", bold ? "0;" : "");
}

void usage()
{
	int w = 66;
	horiz(w, 1);
	fprintf(stderr, "Annotate reference ID with Gene Ontology information\n");
	horiz(w, 0);
	fprintf(stderr, "Usage: \033[31m%s\033[0m -i <in> -o <out> -g <go.odo> -e <egg>\n", __progname);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -i, --in STR     input reference ID list file\n");
	fprintf(stderr, "  -o, --out STR    output GO annotations to file \e[90m[stdout]\e[0m\n");
	fprintf(stderr, "  -g, --go STR     go-basic.odo file\n");
	fprintf(stderr, "  -e, --egg STR    og_info_kegg_go.tsv.gz file\n");
	fprintf(stderr, "\n  -h, --help       print this help message\n");
	fprintf(stderr, "  -v, --version    print program version\n");
	fprintf(stderr, "Contact meta@geneplus.cn for support and bug reports\n");
	horiz(w, 1);
	exit(EXIT_FAILURE);
}

