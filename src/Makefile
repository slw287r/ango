.PHONY: ango

ango: ango.c kstring.c
	cc -O2 -Wall -Wextra -march=native -o $@ $^ -lz

test: ango
	./$^ -i ../dat/seqs.txt -g ../dat/go-basic.obo -e ../dat/e7.og_info_kegg_go.tsv.gz

clean:
	rm -f ango
