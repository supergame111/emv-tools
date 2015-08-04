#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "openemv/scard.h"
#include "openemv/sc_helpers.h"
#include "openemv/tlv.h"
#include "openemv/emv_tags.h"
#include "openemv/dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool print_cb(void *data, const struct tlv *tlv)
{
	emv_tag_dump(tlv, stdout);
	dump_buffer(tlv->value, tlv->len, stdout);

	return true;
}

static struct tlvdb *docmd(struct sc *sc,
		unsigned char cla,
		unsigned char ins,
		unsigned char p1,
		unsigned char p2,
		size_t dlen,
		const unsigned char *data)
{
	unsigned short sw;
	size_t outlen;
	unsigned char *outbuf;
	struct tlvdb *tlvdb = NULL;

	outbuf = sc_command(sc, cla, ins, p1, p2, dlen, data, &sw, &outlen);
	if (!outbuf)
		return NULL;

	if (sw == 0x9000)
		tlvdb = tlvdb_parse(outbuf, outlen);


	free(outbuf);

	return tlvdb;
}

#if 0
static struct {
	size_t len;
	unsigned char aid[16];
} applications[] = {
};
#endif

int main(void)
{
	struct sc *sc;
	const unsigned char pse_name[] = {
		0x31, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31
	};

	sc = scard_init(NULL);
	if (!sc) {
		printf("Cannot init scard\n");
		return 1;
	}

	scard_connect(sc, 0);
	if (scard_is_error(sc)) {
		printf("%s\n", scard_error(sc));
		return 1;
	}

//	struct tlvdb *s;
	struct tlvdb *t;
	const struct tlv *e;

	struct tlvdb *pse = docmd(sc, 0x00, 0xa4, 0x04, 0x00, sizeof(pse_name), pse_name);
	if (!pse)
		return 1;

	e = tlvdb_get(pse, 0x88, NULL);
	if (!e)
		return 1;
	unsigned char sfi = e->value[0];

	int i;
	for (i = 1; ; i++) {
		unsigned short sw;
		size_t outlen;
		unsigned char *outbuf;
		outbuf = sc_command(sc, 0x00, 0xb2, i, (sfi << 3) | 0x04, 0, NULL, &sw, &outlen);
		if (sw == 0x9000) {
			t = tlvdb_parse(outbuf, outlen);
			free(outbuf);
			if (!t)
				return 1;
			tlvdb_add(pse, t);
		} else if (sw == 0x6a83) {
			break;
		} else
			return 1;
	}

#if 0
	for (
		e = tlvdb_get(pse, 0x61, NULL);
		e;
		e = tlvdb_get(pse, 0x61, e)
	    ) {
		print_cb(NULL, e);
	}
#endif


	printf("Final\n");
	tlvdb_visit(pse, print_cb, NULL);
	tlvdb_free(pse);

	scard_disconnect(sc);
	if (scard_is_error(sc)) {
		printf("%s\n", scard_error(sc));
		return 1;
	}
	scard_shutdown(sc);

	return 0;
}