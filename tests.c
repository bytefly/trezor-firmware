/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aes.h"
#include "bignum.h"
#include "base58.h"
#include "bip32.h"
#include "bip39.h"
#include "ecdsa.h"
#include "pbkdf2.h"
#include "rand.h"
#include "sha2.h"
#include "options.h"
#include "curves.h"
#include "secp256k1.h"
#include "nist256p1.h"
#include "ed25519.h"

uint8_t *fromhex(const char *str)
{
	static uint8_t buf[256];
	uint8_t c;
	size_t i;
	for (i = 0; i < strlen(str) / 2; i++) {
		c = 0;
		if (str[i*2] >= '0' && str[i*2] <= '9') c += (str[i*2] - '0') << 4;
		if (str[i*2] >= 'a' && str[i*2] <= 'f') c += (10 + str[i*2] - 'a') << 4;
		if (str[i*2] >= 'A' && str[i*2] <= 'F') c += (10 + str[i*2] - 'A') << 4;
		if (str[i*2+1] >= '0' && str[i*2+1] <= '9') c += (str[i*2+1] - '0');
		if (str[i*2+1] >= 'a' && str[i*2+1] <= 'f') c += (10 + str[i*2+1] - 'a');
		if (str[i*2+1] >= 'A' && str[i*2+1] <= 'F') c += (10 + str[i*2+1] - 'A');
		buf[i] = c;
	}
	return buf;
}

char *tohex(const uint8_t *bin, size_t l)
{
	char *buf = (char *)malloc(l * 2 + 1);
	static char digits[] = "0123456789abcdef";
	size_t i;
	for (i = 0; i < l; i++) {
		buf[i*2  ] = digits[(bin[i] >> 4) & 0xF];
		buf[i*2+1] = digits[bin[i] & 0xF];
	}
	buf[l * 2] = 0;
	return buf;
}

#define _ck_assert_mem(X, Y, L, OP) do { \
  const void* _ck_x = (X); \
  const void* _ck_y = (Y); \
  size_t _ck_l = (L); \
  ck_assert_msg(0 OP memcmp(_ck_y, _ck_x, _ck_l), \
    "Assertion '"#X#OP#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"", tohex(_ck_x, _ck_l), tohex(_ck_y, _ck_l)); \
} while (0)
#define ck_assert_mem_eq(X, Y, L) _ck_assert_mem(X, Y, L, ==)
#define ck_assert_mem_ne(X, Y, L) _ck_assert_mem(X, Y, L, !=)

// from https://github.com/bitcoin/bitcoin/blob/master/src/test/data/base58_keys_valid.json
START_TEST(test_base58)
{
	static const char *base58_vector[] = {
		"0065a16059864a2fdbc7c99a4723a8395bc6f188eb", "1AGNa15ZQXAZUgFiqJ2i7Z2DPU2J6hW62i",
		"0574f209f6ea907e2ea48f74fae05782ae8a665257", "3CMNFxN1oHBc4R1EpboAL5yzHGgE611Xou",
		"6f53c0307d6851aa0ce7825ba883c6bd9ad242b486", "mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs",
		"c46349a418fc4578d10a372b54b45c280cc8c4382f", "2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br",
		"80eddbdc1168f1daeadbd3e44c1e3f8f5a284c2029f78ad26af98583a499de5b19", "5Kd3NBUAdUnhyzenEwVLy9pBKxSwXvE9FMPyR4UKZvpe6E3AgLr",
		"8055c9bccb9ed68446d1b75273bbce89d7fe013a8acd1625514420fb2aca1a21c401", "Kz6UJmQACJmLtaQj5A3JAge4kVTNQ8gbvXuwbmCj7bsaabudb3RD",
		"ef36cb93b9ab1bdabf7fb9f2c04f1b9cc879933530ae7842398eef5a63a56800c2", "9213qJab2HNEpMpYNBa7wHGFKKbkDn24jpANDs2huN3yi4J11ko",
		"efb9f4892c9e8282028fea1d2667c4dc5213564d41fc5783896a0d843fc15089f301", "cTpB4YiyKiBcPxnefsDpbnDxFDffjqJob8wGCEDXxgQ7zQoMXJdH",
		"006d23156cbbdcc82a5a47eee4c2c7c583c18b6bf4", "1Ax4gZtb7gAit2TivwejZHYtNNLT18PUXJ",
		"05fcc5460dd6e2487c7d75b1963625da0e8f4c5975", "3QjYXhTkvuj8qPaXHTTWb5wjXhdsLAAWVy",
		"6ff1d470f9b02370fdec2e6b708b08ac431bf7a5f7", "n3ZddxzLvAY9o7184TB4c6FJasAybsw4HZ",
		"c4c579342c2c4c9220205e2cdc285617040c924a0a", "2NBFNJTktNa7GZusGbDbGKRZTxdK9VVez3n",
		"80a326b95ebae30164217d7a7f57d72ab2b54e3be64928a19da0210b9568d4015e", "5K494XZwps2bGyeL71pWid4noiSNA2cfCibrvRWqcHSptoFn7rc",
		"807d998b45c219a1e38e99e7cbd312ef67f77a455a9b50c730c27f02c6f730dfb401", "L1RrrnXkcKut5DEMwtDthjwRcTTwED36thyL1DebVrKuwvohjMNi",
		"efd6bca256b5abc5602ec2e1c121a08b0da2556587430bcf7e1898af2224885203", "93DVKyFYwSN6wEo3E2fCrFPUp17FtrtNi2Lf7n4G3garFb16CRj",
		"efa81ca4e8f90181ec4b61b6a7eb998af17b2cb04de8a03b504b9e34c4c61db7d901", "cTDVKtMGVYWTHCb1AFjmVbEbWjvKpKqKgMaR3QJxToMSQAhmCeTN",
		"007987ccaa53d02c8873487ef919677cd3db7a6912", "1C5bSj1iEGUgSTbziymG7Cn18ENQuT36vv",
		"0563bcc565f9e68ee0189dd5cc67f1b0e5f02f45cb", "3AnNxabYGoTxYiTEZwFEnerUoeFXK2Zoks",
		"6fef66444b5b17f14e8fae6e7e19b045a78c54fd79", "n3LnJXCqbPjghuVs8ph9CYsAe4Sh4j97wk",
		"c4c3e55fceceaa4391ed2a9677f4a4d34eacd021a0", "2NB72XtkjpnATMggui83aEtPawyyKvnbX2o",
		"80e75d936d56377f432f404aabb406601f892fd49da90eb6ac558a733c93b47252", "5KaBW9vNtWNhc3ZEDyNCiXLPdVPHCikRxSBWwV9NrpLLa4LsXi9",
		"808248bd0375f2f75d7e274ae544fb920f51784480866b102384190b1addfbaa5c01", "L1axzbSyynNYA8mCAhzxkipKkfHtAXYF4YQnhSKcLV8YXA874fgT",
		"ef44c4f6a096eac5238291a94cc24c01e3b19b8d8cef72874a079e00a242237a52", "927CnUkUbasYtDwYwVn2j8GdTuACNnKkjZ1rpZd2yBB1CLcnXpo",
		"efd1de707020a9059d6d3abaf85e17967c6555151143db13dbb06db78df0f15c6901", "cUcfCMRjiQf85YMzzQEk9d1s5A4K7xL5SmBCLrezqXFuTVefyhY7",
		"00adc1cc2081a27206fae25792f28bbc55b831549d", "1Gqk4Tv79P91Cc1STQtU3s1W6277M2CVWu",
		"05188f91a931947eddd7432d6e614387e32b244709", "33vt8ViH5jsr115AGkW6cEmEz9MpvJSwDk",
		"6f1694f5bc1a7295b600f40018a618a6ea48eeb498", "mhaMcBxNh5cqXm4aTQ6EcVbKtfL6LGyK2H",
		"c43b9b3fd7a50d4f08d1a5b0f62f644fa7115ae2f3", "2MxgPqX1iThW3oZVk9KoFcE5M4JpiETssVN",
		"80091035445ef105fa1bb125eccfb1882f3fe69592265956ade751fd095033d8d0", "5HtH6GdcwCJA4ggWEL1B3jzBBUB8HPiBi9SBc5h9i4Wk4PSeApR",
		"80ab2b4bcdfc91d34dee0ae2a8c6b6668dadaeb3a88b9859743156f462325187af01", "L2xSYmMeVo3Zek3ZTsv9xUrXVAmrWxJ8Ua4cw8pkfbQhcEFhkXT8",
		"efb4204389cef18bbe2b353623cbf93e8678fbc92a475b664ae98ed594e6cf0856", "92xFEve1Z9N8Z641KQQS7ByCSb8kGjsDzw6fAmjHN1LZGKQXyMq",
		"efe7b230133f1b5489843260236b06edca25f66adb1be455fbd38d4010d48faeef01", "cVM65tdYu1YK37tNoAyGoJTR13VBYFva1vg9FLuPAsJijGvG6NEA",
		"00c4c1b72491ede1eedaca00618407ee0b772cad0d", "1JwMWBVLtiqtscbaRHai4pqHokhFCbtoB4",
		"05f6fe69bcb548a829cce4c57bf6fff8af3a5981f9", "3QCzvfL4ZRvmJFiWWBVwxfdaNBT8EtxB5y",
		"6f261f83568a098a8638844bd7aeca039d5f2352c0", "mizXiucXRCsEriQCHUkCqef9ph9qtPbZZ6",
		"c4e930e1834a4d234702773951d627cce82fbb5d2e", "2NEWDzHWwY5ZZp8CQWbB7ouNMLqCia6YRda",
		"80d1fab7ab7385ad26872237f1eb9789aa25cc986bacc695e07ac571d6cdac8bc0", "5KQmDryMNDcisTzRp3zEq9e4awRmJrEVU1j5vFRTKpRNYPqYrMg",
		"80b0bbede33ef254e8376aceb1510253fc3550efd0fcf84dcd0c9998b288f166b301", "L39Fy7AC2Hhj95gh3Yb2AU5YHh1mQSAHgpNixvm27poizcJyLtUi",
		"ef037f4192c630f399d9271e26c575269b1d15be553ea1a7217f0cb8513cef41cb", "91cTVUcgydqyZLgaANpf1fvL55FH53QMm4BsnCADVNYuWuqdVys",
		"ef6251e205e8ad508bab5596bee086ef16cd4b239e0cc0c5d7c4e6035441e7d5de01", "cQspfSzsgLeiJGB2u8vrAiWpCU4MxUT6JseWo2SjXy4Qbzn2fwDw",
		"005eadaf9bb7121f0f192561a5a62f5e5f54210292", "19dcawoKcZdQz365WpXWMhX6QCUpR9SY4r",
		"053f210e7277c899c3a155cc1c90f4106cbddeec6e", "37Sp6Rv3y4kVd1nQ1JV5pfqXccHNyZm1x3",
		"6fc8a3c2a09a298592c3e180f02487cd91ba3400b5", "myoqcgYiehufrsnnkqdqbp69dddVDMopJu",
		"c499b31df7c9068d1481b596578ddbb4d3bd90baeb", "2N7FuwuUuoTBrDFdrAZ9KxBmtqMLxce9i1C",
		"80c7666842503db6dc6ea061f092cfb9c388448629a6fe868d068c42a488b478ae", "5KL6zEaMtPRXZKo1bbMq7JDjjo1bJuQcsgL33je3oY8uSJCR5b4",
		"8007f0803fc5399e773555ab1e8939907e9badacc17ca129e67a2f5f2ff84351dd01", "KwV9KAfwbwt51veZWNscRTeZs9CKpojyu1MsPnaKTF5kz69H1UN2",
		"efea577acfb5d1d14d3b7b195c321566f12f87d2b77ea3a53f68df7ebf8604a801", "93N87D6uxSBzwXvpokpzg8FFmfQPmvX4xHoWQe3pLdYpbiwT5YV",
		"ef0b3b34f0958d8a268193a9814da92c3e8b58b4a4378a542863e34ac289cd830c01", "cMxXusSihaX58wpJ3tNuuUcZEQGt6DKJ1wEpxys88FFaQCYjku9h",
		"001ed467017f043e91ed4c44b4e8dd674db211c4e6", "13p1ijLwsnrcuyqcTvJXkq2ASdXqcnEBLE",
		"055ece0cadddc415b1980f001785947120acdb36fc", "3ALJH9Y951VCGcVZYAdpA3KchoP9McEj1G",
		0, 0,
	};
	const char **raw = base58_vector;
	const char **str = base58_vector + 1;
	uint8_t rawn[34];
	char strn[53];
	int r;
	while (*raw && *str) {
		int len = strlen(*raw) / 2;

		memcpy(rawn, fromhex(*raw), len);
		r = base58_encode_check(rawn, len, strn, sizeof(strn));
		ck_assert_int_eq((size_t)r, strlen(*str) + 1);
		ck_assert_str_eq(strn, *str);

		r = base58_decode_check(strn, rawn, len);
		ck_assert_int_eq(r, len);
		ck_assert_mem_eq(rawn,  fromhex(*raw), len);

		raw += 2; str += 2;
	}
}
END_TEST


// test vector 1 from https://en.bitcoin.it/wiki/BIP_0032_TestVectors
START_TEST(test_bip32_vector_1)
{
	HDNode node, node2, node3;
	char str[112];
	int r;

	// init m
	hdnode_from_seed(fromhex("000102030405060708090a0b0c0d0e0f"), 16, SECP256K1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0339a36013301597daef41fbe593a02cc513d0b55527ec2df1050e2e8ff49c85c2"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0']
	hdnode_private_ckd_prime(&node, 0);
	ck_assert_int_eq(node.fingerprint, 0x3442193e);
	ck_assert_mem_eq(node.chain_code,  fromhex("47fdacbd0f1097043b78c63c20c34ef4ed9a111d980047ad16282c7ae6236141"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("edb2e14f9ee77d26dd93b4ecede8d16ed408ce149b6cd80b0715a2d911a0afea"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("035a784662a4a20a65bf6aab9ae98a6c068a81c52e4b032c0fb5400c706cfccc56"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0'/1]
	hdnode_private_ckd(&node, 1);
	ck_assert_int_eq(node.fingerprint, 0x5c1bd648);
	ck_assert_mem_eq(node.chain_code,  fromhex("2a7857631386ba23dacac34180dd1983734e444fdbf774041578e9b6adb37c19"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("3c6cb8d0f6a264c91ea8b5030fadaa8e538b020f0a387421a12de9319dc93368"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03501e454bf00751f24b1b489aa925215d66af2234e3891c3b21a52bedb3cd711c"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0'/1/2']
	hdnode_private_ckd_prime(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0xbef5a2f9);
	ck_assert_mem_eq(node.chain_code,  fromhex("04466b9cc8e161e966409ca52986c584f07e9dc81f735db683c3ff6ec7b1503f"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("cbce0d719ecf7431d88e6a89fa1483e02e35092af60c042b1df2ff59fa424dca"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0357bfe1e341d01c69fe5654309956cbea516822fba8a601743a012a7896ee8dc2"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0'/1/2'/2]
	hdnode_private_ckd(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0xee7ab90c);
	ck_assert_mem_eq(node.chain_code,  fromhex("cfb71883f01676f587d023cc53a35bc7f88f724b1f8c2892ac1275ac822a3edd"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("0f479245fb19a38a1954c5c7c0ebab2f9bdfd96a17563ef28a6a4b1a2a764ef4"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02e8445082a72f29b75ca48748a914df60622a609cacfce8ed0e35804560741d29"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0'/1/2'/2/1000000000]
	hdnode_private_ckd(&node, 1000000000);
	ck_assert_int_eq(node.fingerprint, 0xd880d7d8);
	ck_assert_mem_eq(node.chain_code,  fromhex("c783e67b921d2beb8f6b389cc646d7263b4145701dadd2161548a8b078e65e9e"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("471b76e389e528d6de6d816857e012c5455051cad6660850e58372a6c3e6e7c8"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("022a471424da5e657499d1ff51cb43c47481a03b1e77f951fe64cec9f5a48f7011"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmHScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcYFgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));
}
END_TEST

// test vector 2 from https://en.bitcoin.it/wiki/BIP_0032_TestVectors
START_TEST(test_bip32_vector_2)
{
	HDNode node, node2, node3;
	char str[112];
	int r;

	// init m
	hdnode_from_seed(fromhex("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"), 64, SECP256K1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("60499f801b896d83179a4374aeb7822aaeaceaa0db1f85ee3e904c4defbd9689"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("4b03d6fc340455b363f51020ad3ecca4f0850280cf436c70c727923f6db46c3e"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03cbcaa9c98c877a26977d00825c956a238e8dddfbd322cce4f74b0b5bd6ace4a7"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0]
	r = hdnode_private_ckd(&node, 0);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xbd16bee5);
	ck_assert_mem_eq(node.chain_code,  fromhex("f0909affaa7ee7abe5dd4e100598d4dc53cd709d5a5c2cac40e7412f232f7c9c"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("abe74a98f6c7eabee0428f53798f0ab8aa1bd37873999041703c742f15ac7e1e"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02fc9e5af0ac8d9b3cecfe2a888e2117ba3d089d8585886c9c826b6b22a98d12ea"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0/2147483647']
	r = hdnode_private_ckd_prime(&node, 2147483647);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x5a61ff8e);
	ck_assert_mem_eq(node.chain_code,  fromhex("be17a268474a6bb9c61e1d720cf6215e2a88c5406c4aee7b38547f585c9a37d9"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("877c779ad9687164e9c2f4f0f4ff0340814392330693ce95a58fe18fd52e6e93"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03c01e7425647bdefa82b12d9bad5e3e6865bee0502694b94ca58b666abc0a5c3b"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBrQSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0/2147483647'/1]
	r = hdnode_private_ckd(&node, 1);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xd8ab4937);
	ck_assert_mem_eq(node.chain_code,  fromhex("f366f48f1ea9f2d1d3fe958c95ca84ea18e4c4ddb9366c336c927eb246fb38cb"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("704addf544a06e5ee4bea37098463c23613da32020d604506da8c0518e1da4b7"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03a7d1d856deb74c508e05031f9895dab54626251b3806e16b4bd12e781a7df5b9"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9yog62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJxGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0/2147483647'/1/2147483646']
	r = hdnode_private_ckd_prime(&node, 2147483646);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x78412e3a);
	ck_assert_mem_eq(node.chain_code,  fromhex("637807030d55d01f9a0cb3a7839515d796bd07706386a6eddf06cc29a65a0e29"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("f1c7c871a54a804afe328b4c83a1c33b8e5ff48f5087273f04efa83b247d6a2d"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02d2b36900396c9282fa14628566582f206a5dd0bcc8d5e892611806cafb0301f0"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// [Chain m/0/2147483647'/1/2147483646'/2]
	r = hdnode_private_ckd(&node, 2);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x31a507b8);
	ck_assert_mem_eq(node.chain_code,  fromhex("9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c"), 33);
	hdnode_serialize_private(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	ck_assert_mem_eq(&node, &node2, sizeof(HDNode));
	hdnode_serialize_public(&node, str, sizeof(str));
	ck_assert_str_eq(str,  "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsApME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt");
	r = hdnode_deserialize(str, &node2); ck_assert_int_eq(r, 0);
	memcpy(&node3, &node, sizeof(HDNode));
	memset(&node3.private_key, 0, 32);
	ck_assert_mem_eq(&node2, &node3, sizeof(HDNode));

	// init m
	hdnode_from_seed(fromhex("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"), 64, SECP256K1_NAME, &node);

	// test public derivation
	// [Chain m/0]
	r = hdnode_public_ckd(&node, 0);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xbd16bee5);
	ck_assert_mem_eq(node.chain_code,  fromhex("f0909affaa7ee7abe5dd4e100598d4dc53cd709d5a5c2cac40e7412f232f7c9c"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02fc9e5af0ac8d9b3cecfe2a888e2117ba3d089d8585886c9c826b6b22a98d12ea"), 33);
}
END_TEST

START_TEST(test_bip32_compare)
{
	HDNode node1, node2, node3;
	int i, r;
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node1);
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node2);
	for (i = 0; i < 100; i++) {
		memcpy(&node3, &node1, sizeof(HDNode));
		r = hdnode_private_ckd(&node1, i); ck_assert_int_eq(r, 1);
		r = hdnode_public_ckd(&node2, i);  ck_assert_int_eq(r, 1);
		r = hdnode_public_ckd(&node3, i);  ck_assert_int_eq(r, 1);
		ck_assert_int_eq(node1.depth,       node2.depth);
		ck_assert_int_eq(node1.depth,       node3.depth);
		ck_assert_int_eq(node1.fingerprint, node2.fingerprint);
		ck_assert_int_eq(node1.fingerprint, node3.fingerprint);
		ck_assert_int_eq(node1.child_num,   node2.child_num);
		ck_assert_int_eq(node1.child_num,   node3.child_num);
		ck_assert_mem_eq(node1.chain_code,  node2.chain_code, 32);
		ck_assert_mem_eq(node1.chain_code,  node3.chain_code, 32);
		ck_assert_mem_eq(node2.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
		ck_assert_mem_eq(node3.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
		ck_assert_mem_eq(node1.public_key,  node2.public_key, 33);
		ck_assert_mem_eq(node1.public_key,  node3.public_key, 33);
	}
}
END_TEST

START_TEST(test_bip32_cache_1)
{
	HDNode node1, node2;
	int i, r;

	// test 1 .. 8
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node1);
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node2);

	uint32_t ii[] = {0x80000001, 0x80000002, 0x80000003, 0x80000004, 0x80000005, 0x80000006, 0x80000007, 0x80000008};

	for (i = 0; i < 8; i++) {
		r = hdnode_private_ckd(&node1, ii[i]); ck_assert_int_eq(r, 1);
	}
	r = hdnode_private_ckd_cached(&node2, ii, 8); ck_assert_int_eq(r, 1);
	ck_assert_mem_eq(&node1, &node2, sizeof(HDNode));

	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node1);
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node2);

	// test 1 .. 7, 20
	ii[7] = 20;
	for (i = 0; i < 8; i++) {
		r = hdnode_private_ckd(&node1, ii[i]); ck_assert_int_eq(r, 1);
	}
	r = hdnode_private_ckd_cached(&node2, ii, 8); ck_assert_int_eq(r, 1);
	ck_assert_mem_eq(&node1, &node2, sizeof(HDNode));

	// test different root node
	hdnode_from_seed(fromhex("000000002ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node1);
	hdnode_from_seed(fromhex("000000002ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &node2);

	for (i = 0; i < 8; i++) {
		r = hdnode_private_ckd(&node1, ii[i]); ck_assert_int_eq(r, 1);
	}
	r = hdnode_private_ckd_cached(&node2, ii, 8); ck_assert_int_eq(r, 1);
	ck_assert_mem_eq(&node1, &node2, sizeof(HDNode));
}
END_TEST

START_TEST(test_bip32_cache_2)
{
	HDNode nodea[9], nodeb[9];
	int i, j, r;

	for (j = 0; j < 9; j++) {
		hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &(nodea[j]));
		hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, SECP256K1_NAME, &(nodeb[j]));
	}

	uint32_t ii[] = {0x80000001, 0x80000002, 0x80000003, 0x80000004, 0x80000005, 0x80000006, 0x80000007, 0x80000008};
	for (j = 0; j < 9; j++) {
		// non cached
		for (i = 1; i <= j; i++) {
			r = hdnode_private_ckd(&(nodea[j]), ii[i - 1]); ck_assert_int_eq(r, 1);
		}
		// cached
		r = hdnode_private_ckd_cached(&(nodeb[j]), ii, j); ck_assert_int_eq(r, 1);
	}

	ck_assert_mem_eq(&(nodea[0]), &(nodeb[0]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[1]), &(nodeb[1]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[2]), &(nodeb[2]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[3]), &(nodeb[3]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[4]), &(nodeb[4]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[5]), &(nodeb[5]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[6]), &(nodeb[6]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[7]), &(nodeb[7]), sizeof(HDNode));
	ck_assert_mem_eq(&(nodea[8]), &(nodeb[8]), sizeof(HDNode));
}
END_TEST

START_TEST(test_bip32_nist_seed)
{
	HDNode node;

	// init m
	hdnode_from_seed(fromhex("a7305bc8df8d0951f0cb224c0e95d7707cbdf2c6ce7e8d481fec69c7ff5e9446"), 32, NIST256P1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);	
	ck_assert_mem_eq(node.private_key, fromhex("3b8c18469a4634517d6d0b65448f8e6c62091b45540a1743c5846be55d47d88f"), 32);
	ck_assert_mem_eq(node.chain_code,  fromhex("7762f9729fed06121fd13f326884c82f59aa95c57ac492ce8c9654e60efd130c"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0383619fadcde31063d8c5cb00dbfe1713f3e6fa169d8541a798752a1c1ca0cb20"), 33);

	// init m
	hdnode_from_seed(fromhex("aa305bc8df8d0951f0cb29ad4568d7707cbdf2c6ce7e8d481fec69c7ff5e9446"), 32, NIST256P1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("a81d21f36f987fa0be3b065301bfb6aa9deefbf3dfef6744c37b9a4abc3c68f1"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("0e49dc46ce1d8c29d9b80a05e40f5d0cd68cbf02ae98572186f5343be18084bf"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03aaa4c89acd9a98935330773d3dae55122f3591bac4a40942681768de8df6ba63"), 33);
}
END_TEST

START_TEST(test_bip32_nist_vector_1)
{
	HDNode node;

	// init m
	hdnode_from_seed(fromhex("000102030405060708090a0b0c0d0e0f"), 16, NIST256P1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("beeb672fe4621673f722f38529c07392fecaa61015c80c34f29ce8b41b3cb6ea"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("612091aaa12e22dd2abef664f8a01a82cae99ad7441b7ef8110424915c268bc2"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0266874dc6ade47b3ecd096745ca09bcd29638dd52c2c12117b11ed3e458cfa9e8"), 33);

	// [Chain m/0']
	hdnode_private_ckd_prime(&node, 0);
	ck_assert_int_eq(node.fingerprint, 0xbe6105b5);
	ck_assert_mem_eq(node.chain_code,  fromhex("3460cea53e6a6bb5fb391eeef3237ffd8724bf0a40e94943c98b83825342ee11"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("6939694369114c67917a182c59ddb8cafc3004e63ca5d3b84403ba8613debc0c"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0384610f5ecffe8fda089363a41f56a5c7ffc1d81b59a612d0d649b2d22355590c"), 33);

	// [Chain m/0'/1]
	hdnode_private_ckd(&node, 1);
	ck_assert_int_eq(node.fingerprint, 0x9b02312f);
	ck_assert_mem_eq(node.chain_code,  fromhex("4187afff1aafa8445010097fb99d23aee9f599450c7bd140b6826ac22ba21d0c"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("284e9d38d07d21e4e281b645089a94f4cf5a5a81369acf151a1c3a57f18b2129"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03526c63f8d0b4bbbf9c80df553fe66742df4676b241dabefdef67733e070f6844"), 33);

	// [Chain m/0'/1/2']
	hdnode_private_ckd_prime(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0xb98005c1);
	ck_assert_mem_eq(node.chain_code,  fromhex("98c7514f562e64e74170cc3cf304ee1ce54d6b6da4f880f313e8204c2a185318"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("694596e8a54f252c960eb771a3c41e7e32496d03b954aeb90f61635b8e092aa7"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0359cf160040778a4b14c5f4d7b76e327ccc8c4a6086dd9451b7482b5a4972dda0"), 33);

	// [Chain m/0'/1/2'/2]
	hdnode_private_ckd(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0x0e9f3274);
	ck_assert_mem_eq(node.chain_code,  fromhex("ba96f776a5c3907d7fd48bde5620ee374d4acfd540378476019eab70790c63a0"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("5996c37fd3dd2679039b23ed6f70b506c6b56b3cb5e424681fb0fa64caf82aaa"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("029f871f4cb9e1c97f9f4de9ccd0d4a2f2a171110c61178f84430062230833ff20"), 33);

	// [Chain m/0'/1/2'/2/1000000000]
	hdnode_private_ckd(&node, 1000000000);
	ck_assert_int_eq(node.fingerprint, 0x8b2b5c4b);
	ck_assert_mem_eq(node.chain_code,  fromhex("b9b7b82d326bb9cb5b5b121066feea4eb93d5241103c9e7a18aad40f1dde8059"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("21c4f269ef0a5fd1badf47eeacebeeaa3de22eb8e5b0adcd0f27dd99d34d0119"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02216cd26d31147f72427a453c443ed2cde8a1e53c9cc44e5ddf739725413fe3f4"), 33);
}
END_TEST

START_TEST(test_bip32_nist_vector_2)
{
	HDNode node;
	int r;

	// init m
	hdnode_from_seed(fromhex("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"), 64, NIST256P1_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("96cd4465a9644e31528eda3592aa35eb39a9527769ce1855beafc1b81055e75d"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("eaa31c2e46ca2962227cf21d73a7ef0ce8b31c756897521eb6c7b39796633357"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02c9e16154474b3ed5b38218bb0463e008f89ee03e62d22fdcc8014beab25b48fa"), 33);

	// [Chain m/0]
	r = hdnode_private_ckd(&node, 0);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x607f628f);
	ck_assert_mem_eq(node.chain_code,  fromhex("84e9c258bb8557a40e0d041115b376dd55eda99c0042ce29e81ebe4efed9b86a"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("d7d065f63a62624888500cdb4f88b6d59c2927fee9e6d0cdff9cad555884df6e"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("039b6df4bece7b6c81e2adfeea4bcf5c8c8a6e40ea7ffa3cf6e8494c61a1fc82cc"), 33);

	// [Chain m/0/2147483647']
	r = hdnode_private_ckd_prime(&node, 2147483647);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x946d2a54);
	ck_assert_mem_eq(node.chain_code,  fromhex("f235b2bc5c04606ca9c30027a84f353acf4e4683edbd11f635d0dcc1cd106ea6"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("96d2ec9316746a75e7793684ed01e3d51194d81a42a3276858a5b7376d4b94b9"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02f89c5deb1cae4fedc9905f98ae6cbf6cbab120d8cb85d5bd9a91a72f4c068c76"), 33);

	// [Chain m/0/2147483647'/1]
	r = hdnode_private_ckd(&node, 1);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x218182d8);
	ck_assert_mem_eq(node.chain_code,  fromhex("7c0b833106235e452eba79d2bdd58d4086e663bc8cc55e9773d2b5eeda313f3b"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("974f9096ea6873a915910e82b29d7c338542ccde39d2064d1cc228f371542bbc"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03abe0ad54c97c1d654c1852dfdc32d6d3e487e75fa16f0fd6304b9ceae4220c64"), 33);

	// [Chain m/0/2147483647'/1/2147483646']
	r = hdnode_private_ckd_prime(&node, 2147483646);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x931223e4);
	ck_assert_mem_eq(node.chain_code,  fromhex("5794e616eadaf33413aa309318a26ee0fd5163b70466de7a4512fd4b1a5c9e6a"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("da29649bbfaff095cd43819eda9a7be74236539a29094cd8336b07ed8d4eff63"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("03cb8cb067d248691808cd6b5a5a06b48e34ebac4d965cba33e6dc46fe13d9b933"), 33);

	// [Chain m/0/2147483647'/1/2147483646'/2]
	r = hdnode_private_ckd(&node, 2);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x956c4629);
	ck_assert_mem_eq(node.chain_code,  fromhex("3bfb29ee8ac4484f09db09c2079b520ea5616df7820f071a20320366fbe226a7"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("bb0a77ba01cc31d77205d51d08bd313b979a71ef4de9b062f8958297e746bd67"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("020ee02e18967237cf62672983b253ee62fa4dd431f8243bfeccdf39dbe181387f"), 33);

	// init m
	hdnode_from_seed(fromhex("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"), 64, NIST256P1_NAME, &node);

	// test public derivation
	// [Chain m/0]
	r = hdnode_public_ckd(&node, 0);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x607f628f);
	ck_assert_mem_eq(node.chain_code,  fromhex("84e9c258bb8557a40e0d041115b376dd55eda99c0042ce29e81ebe4efed9b86a"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("039b6df4bece7b6c81e2adfeea4bcf5c8c8a6e40ea7ffa3cf6e8494c61a1fc82cc"), 33);
}
END_TEST

START_TEST(test_bip32_nist_compare)
{
	HDNode node1, node2, node3;
	int i, r;
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, NIST256P1_NAME, &node1);
	hdnode_from_seed(fromhex("301133282ad079cbeb59bc446ad39d333928f74c46997d3609cd3e2801ca69d62788f9f174429946ff4e9be89f67c22fae28cb296a9b37734f75e73d1477af19"), 64, NIST256P1_NAME, &node2);
	for (i = 0; i < 100; i++) {
		memcpy(&node3, &node1, sizeof(HDNode));
		r = hdnode_private_ckd(&node1, i); ck_assert_int_eq(r, 1);
		r = hdnode_public_ckd(&node2, i);  ck_assert_int_eq(r, 1);
		r = hdnode_public_ckd(&node3, i);  ck_assert_int_eq(r, 1);
		ck_assert_int_eq(node1.depth,       node2.depth);
		ck_assert_int_eq(node1.depth,       node3.depth);
		ck_assert_int_eq(node1.fingerprint, node2.fingerprint);
		ck_assert_int_eq(node1.fingerprint, node3.fingerprint);
		ck_assert_int_eq(node1.child_num,   node2.child_num);
		ck_assert_int_eq(node1.child_num,   node3.child_num);
		ck_assert_mem_eq(node1.chain_code,  node2.chain_code, 32);
		ck_assert_mem_eq(node1.chain_code,  node3.chain_code, 32);
		ck_assert_mem_eq(node2.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
		ck_assert_mem_eq(node3.private_key, fromhex("0000000000000000000000000000000000000000000000000000000000000000"), 32);
		ck_assert_mem_eq(node1.public_key,  node2.public_key, 33);
		ck_assert_mem_eq(node1.public_key,  node3.public_key, 33);
	}
}
END_TEST

START_TEST(test_bip32_nist_repeat)
{
	HDNode node, node2;
	int r;

	// init m
	hdnode_from_seed(fromhex("000102030405060708090a0b0c0d0e0f"), 16, NIST256P1_NAME, &node);

	// [Chain m/28578']
	r = hdnode_private_ckd_prime(&node, 28578);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xbe6105b5);
	ck_assert_mem_eq(node.chain_code,  fromhex("e94c8ebe30c2250a14713212f6449b20f3329105ea15b652ca5bdfc68f6c65c2"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("06f0db126f023755d0b8d86d4591718a5210dd8d024e3e14b6159d63f53aa669"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("02519b5554a4872e8c9c1c847115363051ec43e93400e030ba3c36b52a3e70a5b7"), 33);

	memcpy(&node2, &node, sizeof(HDNode));
	r = hdnode_private_ckd(&node2, 33941);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node2.fingerprint, 0x3e2b7bc6);
	ck_assert_mem_eq(node2.chain_code,  fromhex("9e87fe95031f14736774cd82f25fd885065cb7c358c1edf813c72af535e83071"), 32);
	ck_assert_mem_eq(node2.private_key, fromhex("092154eed4af83e078ff9b84322015aefe5769e31270f62c3f66c33888335f3a"), 32);
	ck_assert_mem_eq(node2.public_key,  fromhex("0235bfee614c0d5b2cae260000bb1d0d84b270099ad790022c1ae0b2e782efe120"), 33);

	memcpy(&node2, &node, sizeof(HDNode));
	memset(&node2.private_key, 0, 32);
	r = hdnode_public_ckd(&node2, 33941);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node2.fingerprint, 0x3e2b7bc6);
	ck_assert_mem_eq(node2.chain_code,  fromhex("9e87fe95031f14736774cd82f25fd885065cb7c358c1edf813c72af535e83071"), 32);
	ck_assert_mem_eq(node2.public_key,  fromhex("0235bfee614c0d5b2cae260000bb1d0d84b270099ad790022c1ae0b2e782efe120"), 33);
}
END_TEST

// test vector 1 from https://en.bitcoin.it/wiki/BIP_0032_TestVectors
START_TEST(test_bip32_ed25519_vector_1)
{
	HDNode node;

	// init m
	hdnode_from_seed(fromhex("000102030405060708090a0b0c0d0e0f"), 16, ED25519_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("90046a93de5380a72b5e45010748567d5ea02bbf6522f979e05c0d8d8ca9fffb"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("00a4b2856bfec510abab89753fac1ac0e1112364e7d250545963f135f2a33188ed"), 33);

	// [Chain m/0']
	hdnode_private_ckd_prime(&node, 0);
	ck_assert_int_eq(node.fingerprint, 0xddebc675);
	ck_assert_mem_eq(node.chain_code,  fromhex("8b59aa11380b624e81507a27fedda59fea6d0b779a778918a2fd3590e16e9c69"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("68e0fe46dfb67e368c75379acec591dad19df3cde26e63b93a8e704f1dade7a3"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("008c8a13df77a28f3445213a0f432fde644acaa215fc72dcdf300d5efaa85d350c"), 33);

	// [Chain m/0'/1']
	hdnode_private_ckd_prime(&node, 1);
	ck_assert_int_eq(node.fingerprint, 0x13dab143);
	ck_assert_mem_eq(node.chain_code,  fromhex("a320425f77d1b5c2505a6b1b27382b37368ee640e3557c315416801243552f14"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("b1d0bad404bf35da785a64ca1ac54b2617211d2777696fbffaf208f746ae84f2"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("001932a5270f335bed617d5b935c80aedb1a35bd9fc1e31acafd5372c30f5c1187"), 33);

	// [Chain m/0'/1'/2']
	hdnode_private_ckd_prime(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0xebe4cb29);
	ck_assert_mem_eq(node.chain_code,  fromhex("2e69929e00b5ab250f49c3fb1c12f252de4fed2c1db88387094a0f8c4c9ccd6c"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("92a5b23c0b8a99e37d07df3fb9966917f5d06e02ddbd909c7e184371463e9fc9"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("00ae98736566d30ed0e9d2f4486a64bc95740d89c7db33f52121f8ea8f76ff0fc1"), 33);

	// [Chain m/0'/1'/2'/2']
	hdnode_private_ckd_prime(&node, 2);
	ck_assert_int_eq(node.fingerprint, 0x316ec1c6);
	ck_assert_mem_eq(node.chain_code,  fromhex("8f6d87f93d750e0efccda017d662a1b31a266e4a6f5993b15f5c1f07f74dd5cc"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("30d1dc7e5fc04c31219ab25a27ae00b50f6fd66622f6e9c913253d6511d1e662"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("008abae2d66361c879b900d204ad2cc4984fa2aa344dd7ddc46007329ac76c429c"), 33);

	// [Chain m/0'/1'/2'/2'/1000000000']
	hdnode_private_ckd_prime(&node, 1000000000);
	ck_assert_int_eq(node.fingerprint, 0xd6322ccd);
	ck_assert_mem_eq(node.chain_code,  fromhex("68789923a0cac2cd5a29172a475fe9e0fb14cd6adb5ad98a3fa70333e7afa230"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e8793"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("003c24da049451555d51a7014a37337aa4e12d41e485abccfa46b47dfb2af54b7a"), 33);
}
END_TEST

// test vector 2 from https://en.bitcoin.it/wiki/BIP_0032_TestVectors
START_TEST(test_bip32_ed25519_vector_2)
{
	HDNode node;
	int r;

	// init m
	hdnode_from_seed(fromhex("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542"), 64, ED25519_NAME, &node);

	// [Chain m]
	ck_assert_int_eq(node.fingerprint, 0x00000000);
	ck_assert_mem_eq(node.chain_code,  fromhex("ef70a74db9c3a5af931b5fe73ed8e1a53464133654fd55e7a66f8570b8e33c3b"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("171cb88b1b3c1db25add599712e36245d75bc65a1a5c9e18d76f9f2b1eab4012"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("008fe9693f8fa62a4305a140b9764c5ee01e455963744fe18204b4fb948249308a"), 33);

	// [Chain m/0']
	r = hdnode_private_ckd_prime(&node, 0);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x31981b50);
	ck_assert_mem_eq(node.chain_code,  fromhex("0b78a3226f915c082bf118f83618a618ab6dec793752624cbeb622acb562862d"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("1559eb2bbec5790b0c65d8693e4d0875b1747f4970ae8b650486ed7470845635"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0086fab68dcb57aa196c77c5f264f215a112c22a912c10d123b0d03c3c28ef1037"), 33);

	// [Chain m/0'/2147483647']
	r = hdnode_private_ckd_prime(&node, 2147483647);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x1e9411b1);
	ck_assert_mem_eq(node.chain_code,  fromhex("138f0b2551bcafeca6ff2aa88ba8ed0ed8de070841f0c4ef0165df8181eaad7f"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("ea4f5bfe8694d8bb74b7b59404632fd5968b774ed545e810de9c32a4fb4192f4"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("005ba3b9ac6e90e83effcd25ac4e58a1365a9e35a3d3ae5eb07b9e4d90bcf7506d"), 33);

	// [Chain m/0'/2147483647'/1']
	r = hdnode_private_ckd_prime(&node, 1);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xfcadf38c);
	ck_assert_mem_eq(node.chain_code,  fromhex("73bd9fff1cfbde33a1b846c27085f711c0fe2d66fd32e139d3ebc28e5a4a6b90"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("3757c7577170179c7868353ada796c839135b3d30554bbb74a4b1e4a5a58505c"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("002e66aa57069c86cc18249aecf5cb5a9cebbfd6fadeab056254763874a9352b45"), 33);

	// [Chain m/0'/2147483647'/1'/2147483646']
	r = hdnode_private_ckd_prime(&node, 2147483646);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0xaca70953);
	ck_assert_mem_eq(node.chain_code,  fromhex("0902fe8a29f9140480a00ef244bd183e8a13288e4412d8389d140aac1794825a"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("5837736c89570de861ebc173b1086da4f505d4adb387c6a1b1342d5e4ac9ec72"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("00e33c0f7d81d843c572275f287498e8d408654fdf0d1e065b84e2e6f157aab09b"), 33);

	// [Chain m/0'/2147483647'/1'/2147483646'/2']
	r = hdnode_private_ckd_prime(&node, 2);
	ck_assert_int_eq(r, 1);
	ck_assert_int_eq(node.fingerprint, 0x422c654b);
	ck_assert_mem_eq(node.chain_code,  fromhex("5d70af781f3a37b829f0d060924d5e960bdc02e85423494afc0b1a41bbe196d4"), 32);
	ck_assert_mem_eq(node.private_key, fromhex("551d333177df541ad876a60ea71f00447931c0a9da16f227c11ea080d7391b8d"), 32);
	ck_assert_mem_eq(node.public_key,  fromhex("0047150c75db263559a70d5778bf36abbab30fb061ad69f69ece61a72b0cfa4fc0"), 33);
}
END_TEST

#define test_deterministic(KEY, MSG, K) do {	  \
	sha256_Raw((uint8_t *)MSG, strlen(MSG), buf); \
	res = generate_k_rfc6979(curve, &k, fromhex(KEY), buf); \
	ck_assert_int_eq(res, 0); \
	bn_write_be(&k, buf); \
	ck_assert_mem_eq(buf, fromhex(K), 32); \
} while (0)

START_TEST(test_rfc6979)
{
	int res;
	bignum256 k;
	uint8_t buf[32];
	const ecdsa_curve *curve = &secp256k1;

	test_deterministic("cca9fbcc1b41e5a95d369eaa6ddcff73b61a4efaa279cfc6567e8daa39cbaf50", "sample", "2df40ca70e639d89528a6b670d9d48d9165fdc0febc0974056bdce192b8e16a3");
	test_deterministic("0000000000000000000000000000000000000000000000000000000000000001", "Satoshi Nakamoto", "8f8a276c19f4149656b280621e358cce24f5f52542772691ee69063b74f15d15");
	test_deterministic("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140", "Satoshi Nakamoto", "33a19b60e25fb6f4435af53a3d42d493644827367e6453928554f43e49aa6f90");
	test_deterministic("f8b8af8ce3c7cca5e300d33939540c10d45ce001b8f252bfbc57ba0342904181", "Alan Turing", "525a82b70e67874398067543fd84c83d30c175fdc45fdeee082fe13b1d7cfdf1");
	test_deterministic("0000000000000000000000000000000000000000000000000000000000000001", "All those moments will be lost in time, like tears in rain. Time to die...", "38aa22d72376b4dbc472e06c3ba403ee0a394da63fc58d88686c611aba98d6b3");
	test_deterministic("e91671c46231f833a6406ccbea0e3e392c76c167bac1cb013f6f1013980455c2", "There is a computer disease that anybody who works with computers knows about. It's a very serious disease and it interferes completely with the work. The trouble with computers is that you 'play' with them!", "1f4b84c23a86a221d233f2521be018d9318639d5b8bbd6374a8a59232d16ad3d");
}
END_TEST

START_TEST(test_sign_speed)
{
	uint8_t sig[64], priv_key[32], msg[256];
	size_t i;
	int res;

	for (i = 0; i < sizeof(msg); i++) {
		msg[i] = i * 1103515245;
	}

	const ecdsa_curve *curve = &secp256k1;
	clock_t t = clock();

	memcpy(priv_key, fromhex("c55ece858b0ddd5263f96810fe14437cd3b5e1fbd7c6a2ec1e031f05e86d8bd5"), 32);
	for (i = 0 ; i < 250; i++) {
		res = ecdsa_sign(curve, priv_key, msg, sizeof(msg), sig, 0);
		ck_assert_int_eq(res, 0);
	}

	memcpy(priv_key, fromhex("509a0382ff5da48e402967a671bdcde70046d07f0df52cff12e8e3883b426a0a"), 32);
	for (i = 0 ; i < 250; i++) {
		res = ecdsa_sign(curve, priv_key, msg, sizeof(msg), sig, 0);
		ck_assert_int_eq(res, 0);
	}

	printf("SECP256k1 signing speed: %0.2f sig/s\n", 500.0f / ((float)(clock() - t) / CLOCKS_PER_SEC));

	curve = &nist256p1;
	t = clock();

	memcpy(priv_key, fromhex("c55ece858b0ddd5263f96810fe14437cd3b5e1fbd7c6a2ec1e031f05e86d8bd5"), 32);
	for (i = 0 ; i < 250; i++) {
		res = ecdsa_sign(curve, priv_key, msg, sizeof(msg), sig, 0);
		ck_assert_int_eq(res, 0);
	}

	memcpy(priv_key, fromhex("509a0382ff5da48e402967a671bdcde70046d07f0df52cff12e8e3883b426a0a"), 32);
	for (i = 0 ; i < 250; i++) {
		res = ecdsa_sign(curve, priv_key, msg, sizeof(msg), sig, 0);
		ck_assert_int_eq(res, 0);
	}

	printf("NIST256p1 signing speed: %0.2f sig/s\n", 500.0f / ((float)(clock() - t) / CLOCKS_PER_SEC));
}
END_TEST

START_TEST(test_verify_speed)
{
	uint8_t sig[64], pub_key33[33], pub_key65[65], msg[256];
	size_t i;
	int res;

	for (i = 0; i < sizeof(msg); i++) {
		msg[i] = i * 1103515245;
	}

	const ecdsa_curve *curve = &secp256k1;
	clock_t t = clock();

	memcpy(sig, fromhex("88dc0db6bc5efa762e75fbcc802af69b9f1fcdbdffce748d403f687f855556e610ee8035414099ac7d89cff88a3fa246d332dfa3c78d82c801394112dda039c2"), 64);
	memcpy(pub_key33, fromhex("024054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a0974"), 33);
	memcpy(pub_key65, fromhex("044054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a09746c7083ed2097011b1223a17a644e81f59aa3de22dac119fd980b36a8ff29a244"), 65);

	for (i = 0 ; i < 25; i++) {
		res = ecdsa_verify(curve, pub_key65, sig, msg, sizeof(msg));
		ck_assert_int_eq(res, 0);
		res = ecdsa_verify(curve, pub_key33, sig, msg, sizeof(msg));
		ck_assert_int_eq(res, 0);
	}

	memcpy(sig, fromhex("067040a2adb3d9deefeef95dae86f69671968a0b90ee72c2eab54369612fd524eb6756c5a1bb662f1175a5fa888763cddc3a07b8a045ef6ab358d8d5d1a9a745"), 64);
	memcpy(pub_key33, fromhex("03ff45a5561a76be930358457d113f25fac790794ec70317eff3b97d7080d45719"), 33);
	memcpy(pub_key65, fromhex("04ff45a5561a76be930358457d113f25fac790794ec70317eff3b97d7080d457196235193a15778062ddaa44aef7e6901b781763e52147f2504e268b2d572bf197"), 65);

	for (i = 0 ; i < 25; i++) {
		res = ecdsa_verify(curve, pub_key65, sig, msg, sizeof(msg));
		ck_assert_int_eq(res, 0);
		res = ecdsa_verify(curve, pub_key33, sig, msg, sizeof(msg));
		ck_assert_int_eq(res, 0);
	}

	printf("SECP256k1 verifying speed: %0.2f sig/s\n", 100.0f / ((float)(clock() - t) / CLOCKS_PER_SEC));

	curve = &nist256p1;
	t = clock();

	memcpy(sig, fromhex("7ed26b02c7e9664baaaa1e6ab64b3350c65ff685a7502863f357029c21c92a0dd7f2ae56d229262a8c7e97a0bfe1e7f9fd80c29df97355afa52ca07b507cecb9"), 64);
	memcpy(pub_key33, fromhex("035662d692de519699000c7f5f04b2714e6a7f358c567fbcc001107ab86e4a7dea"), 33);
	memcpy(pub_key65, fromhex("045662d692de519699000c7f5f04b2714e6a7f358c567fbcc001107ab86e4a7dea40523030e793724473d3cd60e436cbfc0a6e4ac5a5d0b340c5d637c6c870c00f"), 65);

	for (i = 0 ; i < 25; i++) {
		res = ecdsa_verify(curve, pub_key65, sig, msg, sizeof(msg));
		//ck_assert_int_eq(res, 0);
		res = ecdsa_verify(curve, pub_key33, sig, msg, sizeof(msg));
		//ck_assert_int_eq(res, 0);
	}

	memcpy(sig, fromhex("6e13956f4227e66264dfcd92cf40b3b253622e96b1e1030392c0b06d9570f725dd1542164a481c0342ceb0b575f3516df536b7b90da57c381dfbd1aac6138c0b"), 64);
	memcpy(pub_key33, fromhex("034b17f6b5c42b1be32c09f49056ee793e67f3ff42058123ce72fffc61d7236bc2"), 33);
	memcpy(pub_key65, fromhex("044b17f6b5c42b1be32c09f49056ee793e67f3ff42058123ce72fffc61d7236bc29b6c29cbbbb2681d2b2e9c699cde8e591650d02bf4bb577ec53fd229442882e5"), 65);

	for (i = 0 ; i < 25; i++) {
		res = ecdsa_verify(curve, pub_key65, sig, msg, sizeof(msg));
		//ck_assert_int_eq(res, 0);
		res = ecdsa_verify(curve, pub_key33, sig, msg, sizeof(msg));
		//ck_assert_int_eq(res, 0);
	}

	printf("NIST256p1 verifying speed: %0.2f sig/s\n", 100.0f / ((float)(clock() - t) / CLOCKS_PER_SEC));
}
END_TEST

// test vectors from http://www.inconteam.com/software-development/41-encryption/55-aes-test-vectors
START_TEST(test_aes)
{
	aes_encrypt_ctx ctxe;
	aes_decrypt_ctx ctxd;
	uint8_t ibuf[16], obuf[16], iv[16], cbuf[16];
	const char **ivp, **plainp, **cipherp;

	// ECB
	static const char *ecb_vector[] = {
		// plain                            cipher
		"6bc1bee22e409f96e93d7e117393172a", "f3eed1bdb5d2a03c064b5a7e3db181f8",
		"ae2d8a571e03ac9c9eb76fac45af8e51", "591ccb10d410ed26dc5ba74a31362870",
		"30c81c46a35ce411e5fbc1191a0a52ef", "b6ed21b99ca6f4f9f153e7b1beafed1d",
		"f69f2445df4f9b17ad2b417be66c3710", "23304b7a39f9f3ff067d8d8f9e24ecc7",
		0, 0,
	};
	plainp = ecb_vector;
	cipherp = ecb_vector + 1;
	while (*plainp && *cipherp) {
		// encrypt
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(ibuf, fromhex(*plainp), 16);
		aes_ecb_encrypt(ibuf, obuf, 16, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*cipherp), 16);
		// decrypt
		aes_decrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxd);
		memcpy(ibuf, fromhex(*cipherp), 16);
		aes_ecb_decrypt(ibuf, obuf, 16, &ctxd);
		ck_assert_mem_eq(obuf, fromhex(*plainp), 16);
		plainp += 2; cipherp += 2;
	}

	// CBC
	static const char *cbc_vector[] = {
		// iv                               plain                               cipher
		"000102030405060708090A0B0C0D0E0F", "6bc1bee22e409f96e93d7e117393172a", "f58c4c04d6e5f1ba779eabfb5f7bfbd6",
		"F58C4C04D6E5F1BA779EABFB5F7BFBD6", "ae2d8a571e03ac9c9eb76fac45af8e51", "9cfc4e967edb808d679f777bc6702c7d",
		"9CFC4E967EDB808D679F777BC6702C7D", "30c81c46a35ce411e5fbc1191a0a52ef", "39f23369a9d9bacfa530e26304231461",
		"39F23369A9D9BACFA530E26304231461", "f69f2445df4f9b17ad2b417be66c3710", "b2eb05e2c39be9fcda6c19078c6a9d1b",
		0, 0, 0,
	};
	ivp = cbc_vector;
	plainp = cbc_vector + 1;
	cipherp = cbc_vector + 2;
	while (*plainp && *cipherp) {
		// encrypt
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*plainp), 16);
		aes_cbc_encrypt(ibuf, obuf, 16, iv, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*cipherp), 16);
		// decrypt
		aes_decrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxd);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*cipherp), 16);
		aes_cbc_decrypt(ibuf, obuf, 16, iv, &ctxd);
		ck_assert_mem_eq(obuf, fromhex(*plainp), 16);
		ivp += 3; plainp += 3; cipherp += 3;
	}

	// CFB
	static const char *cfb_vector[] = {
		"000102030405060708090A0B0C0D0E0F", "6bc1bee22e409f96e93d7e117393172a", "DC7E84BFDA79164B7ECD8486985D3860",
		"DC7E84BFDA79164B7ECD8486985D3860", "ae2d8a571e03ac9c9eb76fac45af8e51", "39ffed143b28b1c832113c6331e5407b",
		"39FFED143B28B1C832113C6331E5407B", "30c81c46a35ce411e5fbc1191a0a52ef", "df10132415e54b92a13ed0a8267ae2f9",
		"DF10132415E54B92A13ED0A8267AE2F9", "f69f2445df4f9b17ad2b417be66c3710", "75a385741ab9cef82031623d55b1e471",
		0, 0, 0,
	};
	ivp = cfb_vector;
	plainp = cfb_vector + 1;
	cipherp = cfb_vector + 2;
	while (*plainp && *cipherp) {
		// encrypt
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*plainp), 16);
		aes_cfb_encrypt(ibuf, obuf, 16, iv, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*cipherp), 16);
		// decrypt (uses encryption)
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*cipherp), 16);
		aes_cfb_decrypt(ibuf, obuf, 16, iv, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*plainp), 16);
		ivp += 3; plainp += 3; cipherp += 3;
	}

	// OFB
	static const char *ofb_vector[] = {
		"000102030405060708090A0B0C0D0E0F", "6bc1bee22e409f96e93d7e117393172a", "dc7e84bfda79164b7ecd8486985d3860",
		"B7BF3A5DF43989DD97F0FA97EBCE2F4A", "ae2d8a571e03ac9c9eb76fac45af8e51", "4febdc6740d20b3ac88f6ad82a4fb08d",
		"E1C656305ED1A7A6563805746FE03EDC", "30c81c46a35ce411e5fbc1191a0a52ef", "71ab47a086e86eedf39d1c5bba97c408",
		"41635BE625B48AFC1666DD42A09D96E7", "f69f2445df4f9b17ad2b417be66c3710", "0126141d67f37be8538f5a8be740e484",
		0, 0, 0,
	};
	ivp = ofb_vector;
	plainp = ofb_vector + 1;
	cipherp = ofb_vector + 2;
	while (*plainp && *cipherp) {
		// encrypt
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*plainp), 16);
		aes_ofb_encrypt(ibuf, obuf, 16, iv, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*cipherp), 16);
		// decrypt (uses encryption)
		aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
		memcpy(iv, fromhex(*ivp), 16);
		memcpy(ibuf, fromhex(*cipherp), 16);
		aes_ofb_decrypt(ibuf, obuf, 16, iv, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*plainp), 16);
		ivp += 3; plainp += 3; cipherp += 3;
	}

	// CTR
	static const char *ctr_vector[] = {
		// plain                            cipher
		"6bc1bee22e409f96e93d7e117393172a", "601ec313775789a5b7a7f504bbf3d228",
		"ae2d8a571e03ac9c9eb76fac45af8e51", "f443e3ca4d62b59aca84e990cacaf5c5",
		"30c81c46a35ce411e5fbc1191a0a52ef", "2b0930daa23de94ce87017ba2d84988d",
		"f69f2445df4f9b17ad2b417be66c3710", "dfc9c58db67aada613c2dd08457941a6",
		0, 0,
	};
	// encrypt
	plainp = ctr_vector;
	cipherp = ctr_vector + 1;
	memcpy(cbuf, fromhex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"), 16);
	aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
	while (*plainp && *cipherp) {
		memcpy(ibuf, fromhex(*plainp), 16);
		aes_ctr_encrypt(ibuf, obuf, 16, cbuf, aes_ctr_cbuf_inc, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*cipherp), 16);
		plainp += 2; cipherp += 2;
	}
	// decrypt (uses encryption)
	plainp = ctr_vector;
	cipherp = ctr_vector + 1;
	memcpy(cbuf, fromhex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"), 16);
	aes_encrypt_key256(fromhex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"), &ctxe);
	while (*plainp && *cipherp) {
		memcpy(ibuf, fromhex(*cipherp), 16);
		aes_ctr_decrypt(ibuf, obuf, 16, cbuf, aes_ctr_cbuf_inc, &ctxe);
		ck_assert_mem_eq(obuf, fromhex(*plainp), 16);
		plainp += 2; cipherp += 2;
	}
}
END_TEST

// test vectors from https://stackoverflow.com/questions/5130513/pbkdf2-hmac-sha2-test-vectors
START_TEST(test_pbkdf2_hmac_sha256)
{
	uint8_t k[40], s[40];

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha256((uint8_t *)"password", 8, s, 4, 1, k, 32, 0);
	ck_assert_mem_eq(k, fromhex("120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b"), 32);

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha256((uint8_t *)"password", 8, s, 4, 2, k, 32, 0);
	ck_assert_mem_eq(k, fromhex("ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43"), 32);

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha256((uint8_t *)"password", 8, s, 4, 4096, k, 32, 0);
	ck_assert_mem_eq(k, fromhex("c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a"), 32);

	strcpy((char *)s, "saltSALTsaltSALTsaltSALTsaltSALTsalt");
	pbkdf2_hmac_sha256((uint8_t *)"passwordPASSWORDpassword", 3*8, s, 9*4, 4096, k, 40, 0);
	ck_assert_mem_eq(k, fromhex("348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9"), 40);
}
END_TEST

// test vectors from http://stackoverflow.com/questions/15593184/pbkdf2-hmac-sha-512-test-vectors
START_TEST(test_pbkdf2_hmac_sha512)
{
	uint8_t k[64], s[40];

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha512((uint8_t *)"password", 8, s, 4, 1, k, 64, 0);
	ck_assert_mem_eq(k, fromhex("867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce"), 64);

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha512((uint8_t *)"password", 8, s, 4, 2, k, 64, 0);
	ck_assert_mem_eq(k, fromhex("e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53cf76cab2868a39b9f7840edce4fef5a82be67335c77a6068e04112754f27ccf4e"), 64);

	strcpy((char *)s, "salt");
	pbkdf2_hmac_sha512((uint8_t *)"password", 8, s, 4, 4096, k, 64, 0);
	ck_assert_mem_eq(k, fromhex("d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f87f6902e072f457b5143f30602641b3d55cd335988cb36b84376060ecd532e039b742a239434af2d5"), 64);

	strcpy((char *)s, "saltSALTsaltSALTsaltSALTsaltSALTsalt");
	pbkdf2_hmac_sha512((uint8_t *)"passwordPASSWORDpassword", 3*8, s, 9*4, 4096, k, 64, 0);
	ck_assert_mem_eq(k, fromhex("8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b868c005174dc4ee71115b59f9e60cd9532fa33e0f75aefe30225c583a186cd82bd4daea9724a3d3b8"), 64);
}
END_TEST

START_TEST(test_mnemonic)
{
	static const char *vectors[] = {
		"00000000000000000000000000000000",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
		"c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04",
		"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
		"legal winner thank year wave sausage worth useful legal winner thank yellow",
		"2e8905819b8723fe2c1d161860e5ee1830318dbf49a83bd451cfb8440c28bd6fa457fe1296106559a3c80937a1c1069be3a3a5bd381ee6260e8d9739fce1f607",
		"80808080808080808080808080808080",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
		"d71de856f81a8acc65e6fc851a38d4d7ec216fd0796d0a6827a3ad6ed5511a30fa280f12eb2e47ed2ac03b5c462a0358d18d69fe4f985ec81778c1b370b652a8",
		"ffffffffffffffffffffffffffffffff",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
		"ac27495480225222079d7be181583751e86f571027b0497b5b5d11218e0a8a13332572917f0f8e5a589620c6f15b11c61dee327651a14c34e18231052e48c069",
		"000000000000000000000000000000000000000000000000",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
		"035895f2f481b1b0f01fcf8c289c794660b289981a78f8106447707fdd9666ca06da5a9a565181599b79f53b844d8a71dd9f439c52a3d7b3e8a79c906ac845fa",
		"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
		"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
		"f2b94508732bcbacbcc020faefecfc89feafa6649a5491b8c952cede496c214a0c7b3c392d168748f2d4a612bada0753b52a1c7ac53c1e93abd5c6320b9e95dd",
		"808080808080808080808080808080808080808080808080",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
		"107d7c02a5aa6f38c58083ff74f04c607c2d2c0ecc55501dadd72d025b751bc27fe913ffb796f841c49b1d33b610cf0e91d3aa239027f5e99fe4ce9e5088cd65",
		"ffffffffffffffffffffffffffffffffffffffffffffffff",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
		"0cd6e5d827bb62eb8fc1e262254223817fd068a74b5b449cc2f667c3f1f985a76379b43348d952e2265b4cd129090758b3e3c2c49103b5051aac2eaeb890a528",
		"0000000000000000000000000000000000000000000000000000000000000000",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
		"bda85446c68413707090a52022edd26a1c9462295029f2e60cd7c4f2bbd3097170af7a4d73245cafa9c3cca8d561a7c3de6f5d4a10be8ed2a5e608d68f92fcc8",
		"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
		"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
		"bc09fca1804f7e69da93c2f2028eb238c227f2e9dda30cd63699232578480a4021b146ad717fbb7e451ce9eb835f43620bf5c514db0f8add49f5d121449d3e87",
		"8080808080808080808080808080808080808080808080808080808080808080",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
		"c0c519bd0e91a2ed54357d9d1ebef6f5af218a153624cf4f2da911a0ed8f7a09e2ef61af0aca007096df430022f7a2b6fb91661a9589097069720d015e4e982f",
		"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
		"dd48c104698c30cfe2b6142103248622fb7bb0ff692eebb00089b32d22484e1613912f0a5b694407be899ffd31ed3992c456cdf60f5d4564b8ba3f05a69890ad",
		"77c2b00716cec7213839159e404db50d",
		"jelly better achieve collect unaware mountain thought cargo oxygen act hood bridge",
		"b5b6d0127db1a9d2226af0c3346031d77af31e918dba64287a1b44b8ebf63cdd52676f672a290aae502472cf2d602c051f3e6f18055e84e4c43897fc4e51a6ff",
		"b63a9c59a6e641f288ebc103017f1da9f8290b3da6bdef7b",
		"renew stay biology evidence goat welcome casual join adapt armor shuffle fault little machine walk stumble urge swap",
		"9248d83e06f4cd98debf5b6f010542760df925ce46cf38a1bdb4e4de7d21f5c39366941c69e1bdbf2966e0f6e6dbece898a0e2f0a4c2b3e640953dfe8b7bbdc5",
		"3e141609b97933b66a060dcddc71fad1d91677db872031e85f4c015c5e7e8982",
		"dignity pass list indicate nasty swamp pool script soccer toe leaf photo multiply desk host tomato cradle drill spread actor shine dismiss champion exotic",
		"ff7f3184df8696d8bef94b6c03114dbee0ef89ff938712301d27ed8336ca89ef9635da20af07d4175f2bf5f3de130f39c9d9e8dd0472489c19b1a020a940da67",
		"0460ef47585604c5660618db2e6a7e7f",
		"afford alter spike radar gate glance object seek swamp infant panel yellow",
		"65f93a9f36b6c85cbe634ffc1f99f2b82cbb10b31edc7f087b4f6cb9e976e9faf76ff41f8f27c99afdf38f7a303ba1136ee48a4c1e7fcd3dba7aa876113a36e4",
		"72f60ebac5dd8add8d2a25a797102c3ce21bc029c200076f",
		"indicate race push merry suffer human cruise dwarf pole review arch keep canvas theme poem divorce alter left",
		"3bbf9daa0dfad8229786ace5ddb4e00fa98a044ae4c4975ffd5e094dba9e0bb289349dbe2091761f30f382d4e35c4a670ee8ab50758d2c55881be69e327117ba",
		"2c85efc7f24ee4573d2b81a6ec66cee209b2dcbd09d8eddc51e0215b0b68e416",
		"clutch control vehicle tonight unusual clog visa ice plunge glimpse recipe series open hour vintage deposit universe tip job dress radar refuse motion taste",
		"fe908f96f46668b2d5b37d82f558c77ed0d69dd0e7e043a5b0511c48c2f1064694a956f86360c93dd04052a8899497ce9e985ebe0c8c52b955e6ae86d4ff4449",
		"eaebabb2383351fd31d703840b32e9e2",
		"turtle front uncle idea crush write shrug there lottery flower risk shell",
		"bdfb76a0759f301b0b899a1e3985227e53b3f51e67e3f2a65363caedf3e32fde42a66c404f18d7b05818c95ef3ca1e5146646856c461c073169467511680876c",
		"7ac45cfe7722ee6c7ba84fbc2d5bd61b45cb2fe5eb65aa78",
		"kiss carry display unusual confirm curtain upgrade antique rotate hello void custom frequent obey nut hole price segment",
		"ed56ff6c833c07982eb7119a8f48fd363c4a9b1601cd2de736b01045c5eb8ab4f57b079403485d1c4924f0790dc10a971763337cb9f9c62226f64fff26397c79",
		"4fa1a8bc3e6d80ee1316050e862c1812031493212b7ec3f3bb1b08f168cabeef",
		"exile ask congress lamp submit jacket era scheme attend cousin alcohol catch course end lucky hurt sentence oven short ball bird grab wing top",
		"095ee6f817b4c2cb30a5a797360a81a40ab0f9a4e25ecd672a3f58a0b5ba0687c096a6b14d2c0deb3bdefce4f61d01ae07417d502429352e27695163f7447a8c",
		"18ab19a9f54a9274f03e5209a2ac8a91",
		"board flee heavy tunnel powder denial science ski answer betray cargo cat",
		"6eff1bb21562918509c73cb990260db07c0ce34ff0e3cc4a8cb3276129fbcb300bddfe005831350efd633909f476c45c88253276d9fd0df6ef48609e8bb7dca8",
		"18a2e1d81b8ecfb2a333adcb0c17a5b9eb76cc5d05db91a4",
		"board blade invite damage undo sun mimic interest slam gaze truly inherit resist great inject rocket museum chief",
		"f84521c777a13b61564234bf8f8b62b3afce27fc4062b51bb5e62bdfecb23864ee6ecf07c1d5a97c0834307c5c852d8ceb88e7c97923c0a3b496bedd4e5f88a9",
		"15da872c95a13dd738fbf50e427583ad61f18fd99f628c417a61cf8343c90419",
		"beyond stage sleep clip because twist token leaf atom beauty genius food business side grid unable middle armed observe pair crouch tonight away coconut",
		"b15509eaa2d09d3efd3e006ef42151b30367dc6e3aa5e44caba3fe4d3e352e65101fbdb86a96776b91946ff06f8eac594dc6ee1d3e82a42dfe1b40fef6bcc3fd",
		0,
		0,
		0,
	};

	const char **a, **b, **c, *m;
	uint8_t seed[64];

	a = vectors;
	b = vectors + 1;
	c = vectors + 2;
	while (*a && *b && *c) {
		m = mnemonic_from_data(fromhex(*a), strlen(*a) / 2);
		ck_assert_str_eq(m, *b);
		mnemonic_to_seed(m, "TREZOR", seed, 0);
		ck_assert_mem_eq(seed, fromhex(*c), strlen(*c) / 2);
#if USE_BIP39_CACHE
		// try second time to check whether caching results work
		mnemonic_to_seed(m, "TREZOR", seed, 0);
		ck_assert_mem_eq(seed, fromhex(*c), strlen(*c) / 2);
#endif
		a += 3; b += 3; c += 3;
	}
}
END_TEST

START_TEST(test_mnemonic_check)
{
	static const char *vectors_ok[] = {
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
		"legal winner thank year wave sausage worth useful legal winner thank yellow",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
		"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
		"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
		"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
		"jelly better achieve collect unaware mountain thought cargo oxygen act hood bridge",
		"renew stay biology evidence goat welcome casual join adapt armor shuffle fault little machine walk stumble urge swap",
		"dignity pass list indicate nasty swamp pool script soccer toe leaf photo multiply desk host tomato cradle drill spread actor shine dismiss champion exotic",
		"afford alter spike radar gate glance object seek swamp infant panel yellow",
		"indicate race push merry suffer human cruise dwarf pole review arch keep canvas theme poem divorce alter left",
		"clutch control vehicle tonight unusual clog visa ice plunge glimpse recipe series open hour vintage deposit universe tip job dress radar refuse motion taste",
		"turtle front uncle idea crush write shrug there lottery flower risk shell",
		"kiss carry display unusual confirm curtain upgrade antique rotate hello void custom frequent obey nut hole price segment",
		"exile ask congress lamp submit jacket era scheme attend cousin alcohol catch course end lucky hurt sentence oven short ball bird grab wing top",
		"board flee heavy tunnel powder denial science ski answer betray cargo cat",
		"board blade invite damage undo sun mimic interest slam gaze truly inherit resist great inject rocket museum chief",
		"beyond stage sleep clip because twist token leaf atom beauty genius food business side grid unable middle armed observe pair crouch tonight away coconut",
		0,
	};
	static const char *vectors_fail[] = {
		"above abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
		"above winner thank year wave sausage worth useful legal winner thank yellow",
		"above advice cage absurd amount doctor acoustic avoid letter advice cage above",
		"above zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
		"above abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
		"above winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
		"above advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
		"above zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
		"above abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
		"above winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
		"above advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
		"above zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
		"above better achieve collect unaware mountain thought cargo oxygen act hood bridge",
		"above stay biology evidence goat welcome casual join adapt armor shuffle fault little machine walk stumble urge swap",
		"above pass list indicate nasty swamp pool script soccer toe leaf photo multiply desk host tomato cradle drill spread actor shine dismiss champion exotic",
		"above alter spike radar gate glance object seek swamp infant panel yellow",
		"above race push merry suffer human cruise dwarf pole review arch keep canvas theme poem divorce alter left",
		"above control vehicle tonight unusual clog visa ice plunge glimpse recipe series open hour vintage deposit universe tip job dress radar refuse motion taste",
		"above front uncle idea crush write shrug there lottery flower risk shell",
		"above carry display unusual confirm curtain upgrade antique rotate hello void custom frequent obey nut hole price segment",
		"above ask congress lamp submit jacket era scheme attend cousin alcohol catch course end lucky hurt sentence oven short ball bird grab wing top",
		"above flee heavy tunnel powder denial science ski answer betray cargo cat",
		"above blade invite damage undo sun mimic interest slam gaze truly inherit resist great inject rocket museum chief",
		"above stage sleep clip because twist token leaf atom beauty genius food business side grid unable middle armed observe pair crouch tonight away coconut",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
		"winner thank year wave sausage worth useful legal winner thank yellow",
		"advice cage absurd amount doctor acoustic avoid letter advice cage above",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent",
		"winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will",
		"advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when",
		"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art",
		"winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title",
		"advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless",
		"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote",
		"better achieve collect unaware mountain thought cargo oxygen act hood bridge",
		"stay biology evidence goat welcome casual join adapt armor shuffle fault little machine walk stumble urge swap",
		"pass list indicate nasty swamp pool script soccer toe leaf photo multiply desk host tomato cradle drill spread actor shine dismiss champion exotic",
		"alter spike radar gate glance object seek swamp infant panel yellow",
		"race push merry suffer human cruise dwarf pole review arch keep canvas theme poem divorce alter left",
		"control vehicle tonight unusual clog visa ice plunge glimpse recipe series open hour vintage deposit universe tip job dress radar refuse motion taste",
		"front uncle idea crush write shrug there lottery flower risk shell",
		"carry display unusual confirm curtain upgrade antique rotate hello void custom frequent obey nut hole price segment",
		"ask congress lamp submit jacket era scheme attend cousin alcohol catch course end lucky hurt sentence oven short ball bird grab wing top",
		"flee heavy tunnel powder denial science ski answer betray cargo cat",
		"blade invite damage undo sun mimic interest slam gaze truly inherit resist great inject rocket museum chief",
		"stage sleep clip because twist token leaf atom beauty genius food business side grid unable middle armed observe pair crouch tonight away coconut",
		0,
	};

	const char **m;
	int r;
	m = vectors_ok;
	while (*m) {
		r = mnemonic_check(*m);
		ck_assert_int_eq(r, 1);
		m++;
	}
	m = vectors_fail;
	while (*m) {
		r = mnemonic_check(*m);
		ck_assert_int_eq(r, 0);
		m++;
	}
}
END_TEST

START_TEST(test_address)
{
	char address[36];
	uint8_t pub_key[65];

	memcpy(pub_key, fromhex("0226659c1cf7321c178c07437150639ff0c5b7679c7ea195253ed9abda2e081a37"), 33);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "139MaMHp3Vjo8o4x8N1ZLWEtovLGvBsg6s");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mhfJsQNnrXB3uuYZqvywARTDfuvyjg4RBh");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "MxiimznnxsqMfLKTQBL8Z2PoY9jKpjgkCu");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LMNJqZbe89yrPbm7JVzrcXJf28hZ1rKPaH");

	memcpy(pub_key, fromhex("025b1654a0e78d28810094f6c5a96b8efb8a65668b578f170ac2b1f83bc63ba856"), 33);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "19Ywfm3witp6C1yBMy4NRYHY2347WCRBfQ");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mp4txp8vXvFLy8So5Y2kFTVrt2epN6YzdP");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "N58JsQYveGueiZDgdnNwe4SSkGTAToutAY");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LTmtvyMmoZ49SpfLY73fhZMJEFRPdyohKh");

	memcpy(pub_key, fromhex("03433f246a12e6486a51ff08802228c61cf895175a9b49ed4766ea9a9294a3c7fe"), 33);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "1FWE2bn3MWhc4QidcF6AvEWpK77sSi2cAP");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mv2BKes2AY8rqXCFKp4Yk9j9B6iaMfWRLN");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "NB5bEFH2GtoAawy8t4Qk8kfj3LWvQs3MhB");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LZjBHp5sSAwfKDQnnP5UCFaaXKV9YheGxQ");

	memcpy(pub_key, fromhex("03aeb03abeee0f0f8b4f7a5d65ce31f9570cef9f72c2dd8a19b4085a30ab033d48"), 33);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "1yrZb8dhdevoqpUEGi2tUccUEeiMKeLcs");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mgVoreDcWf6BaxJ5wqgQiPpwLEFRLSr8U8");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "MwZDmEdcd1kVLP4yW62c6zmXCU3mNbveDo");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LLCopoSTnHtz4eWdQQhLAVgNgT1zTi4QBK");

	memcpy(pub_key, fromhex("0496e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a016054201c025e2a5d96b1629b95194e806c63eb96facaedc733b1a4b70ab3b33e3a"), 65);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "194SZbL75xCCGBbKtMsyWLE5r9s2V6mhVM");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "moaPreR5tydT3J4wbvrMLFSQi9TjPCiZc6");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "N4domEq61LHkniqqABCYirNzaPG5NRU8GH");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LTHPpodwAcSFWzHV4VsGnMHr4NEJajMnKX");

	memcpy(pub_key, fromhex("0498010f8a687439ff497d3074beb4519754e72c4b6220fb669224749591dde416f3961f8ece18f8689bb32235e436874d2174048b86118a00afbd5a4f33a24f0f"), 65);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "1A2WfBD4BJFwYHFPc5KgktqtbdJLBuVKc4");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mpYTxEJ2zKhCKPj1KeJ4ap4DTcu39T3uzD");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "N5bsrpi36gMW4pVtsteFyQzoKrhPE7nkxK");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LUFTvPWtFxVzo5wYnDJz2uueoqfcMYiuxH");

	memcpy(pub_key, fromhex("04f80490839af36d13701ec3f9eebdac901b51c362119d74553a3c537faff31b17e2a59ebddbdac9e87b816307a7ed5b826b8f40b92719086238e1bebf19b77a4d"), 65);
	ecdsa_get_address(pub_key,   0, address, sizeof(address)); ck_assert_str_eq(address, "19J81hrPnQxg9UGx45ibTieCkb2ttm8CLL");
	ecdsa_get_address(pub_key, 111, address, sizeof(address)); ck_assert_str_eq(address, "mop5JkwNbSPvvakZmegyHdrXcadbjLazww");
	ecdsa_get_address(pub_key,  52, address, sizeof(address)); ck_assert_str_eq(address, "N4sVDMMNho4Eg1XTKu3AgEo7UpRwq3aNbn");
	ecdsa_get_address(pub_key,  48, address, sizeof(address)); ck_assert_str_eq(address, "LTX5GvADs5CjQGy7EDhtjjhxxoQB2Uhicd");
}
END_TEST

START_TEST(test_pubkey_validity)
{
	uint8_t pub_key[65];
	curve_point pub;
	int res;
	const ecdsa_curve *curve = &secp256k1;

	memcpy(pub_key, fromhex("0226659c1cf7321c178c07437150639ff0c5b7679c7ea195253ed9abda2e081a37"), 33);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("025b1654a0e78d28810094f6c5a96b8efb8a65668b578f170ac2b1f83bc63ba856"), 33);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("03433f246a12e6486a51ff08802228c61cf895175a9b49ed4766ea9a9294a3c7fe"), 33);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("03aeb03abeee0f0f8b4f7a5d65ce31f9570cef9f72c2dd8a19b4085a30ab033d48"), 33);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("0496e8f2093f018aff6c2e2da5201ee528e2c8accbf9cac51563d33a7bb74a016054201c025e2a5d96b1629b95194e806c63eb96facaedc733b1a4b70ab3b33e3a"), 65);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("0498010f8a687439ff497d3074beb4519754e72c4b6220fb669224749591dde416f3961f8ece18f8689bb32235e436874d2174048b86118a00afbd5a4f33a24f0f"), 65);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("04f80490839af36d13701ec3f9eebdac901b51c362119d74553a3c537faff31b17e2a59ebddbdac9e87b816307a7ed5b826b8f40b92719086238e1bebf19b77a4d"), 65);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 1);

	memcpy(pub_key, fromhex("04f80490839af36d13701ec3f9eebdac901b51c362119d74553a3c537faff31b17e2a59ebddbdac9e87b816307a7ed5b826b8f40b92719086238e1bebf00000000"), 65);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 0);

	memcpy(pub_key, fromhex("04f80490839af36d13701ec3f9eebdac901b51c362119d74553a3c537faff31b17e2a59ebddbdac9e87b816307a7ed5b8211111111111111111111111111111111"), 65);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 0);

	memcpy(pub_key, fromhex("00"), 1);
	res = ecdsa_read_pubkey(curve, pub_key, &pub);
	ck_assert_int_eq(res, 0);
}
END_TEST

START_TEST(test_wif)
{
	uint8_t priv_key[32];
	char wif[53];

	memcpy(priv_key, fromhex("1111111111111111111111111111111111111111111111111111111111111111"), 32);
	ecdsa_get_wif(priv_key, 0x80, wif, sizeof(wif)); ck_assert_str_eq(wif, "KwntMbt59tTsj8xqpqYqRRWufyjGunvhSyeMo3NTYpFYzZbXJ5Hp");
	ecdsa_get_wif(priv_key, 0xEF, wif, sizeof(wif)); ck_assert_str_eq(wif, "cN9spWsvaxA8taS7DFMxnk1yJD2gaF2PX1npuTpy3vuZFJdwavaw");

	memcpy(priv_key, fromhex("dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"), 32);
	ecdsa_get_wif(priv_key, 0x80, wif, sizeof(wif)); ck_assert_str_eq(wif, "L4ezQvyC6QoBhxB4GVs9fAPhUKtbaXYUn8YTqoeXwbevQq4U92vN");
	ecdsa_get_wif(priv_key, 0xEF, wif, sizeof(wif)); ck_assert_str_eq(wif, "cV1ysqy3XUVSsPeKeugH2Utm6ZC1EyeArAgvxE73SiJvfa6AJng7");

	memcpy(priv_key, fromhex("47f7616ea6f9b923076625b4488115de1ef1187f760e65f89eb6f4f7ff04b012"), 32);
	ecdsa_get_wif(priv_key, 0x80, wif, sizeof(wif)); ck_assert_str_eq(wif, "KydbzBtk6uc7M6dXwEgTEH2sphZxSPbmDSz6kUUHi4eUpSQuhEbq");
	ecdsa_get_wif(priv_key, 0xEF, wif, sizeof(wif)); ck_assert_str_eq(wif, "cPzbT6tbXyJNWY6oKeVabbXwSvsN6qhTHV8ZrtvoDBJV5BRY1G5Q");
}
END_TEST

START_TEST(test_address_decode)
{
	int res;
	uint8_t decode[21];
	// byte      0 : address type
	// bytes  1-20 : pubkey hash 160
	res = ecdsa_address_decode("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("00c4c5d791fcb4654a1ef5e03fe0ad3d9c598f9827"), 21);

	res = ecdsa_address_decode("myTPjxggahXyAzuMcYp5JTkbybANyLsYBW", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("6fc4c5d791fcb4654a1ef5e03fe0ad3d9c598f9827"), 21);

	res = ecdsa_address_decode("NEWoeZ6gh4CGvRgFAoAGh4hBqpxizGT6gZ", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("34c4c5d791fcb4654a1ef5e03fe0ad3d9c598f9827"), 21);

	res = ecdsa_address_decode("LdAPi7uXrLLmeh7u57pzkZc3KovxEDYRJq", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("30c4c5d791fcb4654a1ef5e03fe0ad3d9c598f9827"), 21);

	res = ecdsa_address_decode("1C7zdTfnkzmr13HfA2vNm5SJYRK6nEKyq8", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("0079fbfc3f34e7745860d76137da68f362380c606c"), 21);

	res = ecdsa_address_decode("mrdwvWkma2D6n9mGsbtkazedQQuoksnqJV", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("6f79fbfc3f34e7745860d76137da68f362380c606c"), 21);

	res = ecdsa_address_decode("N7hMq7AmgNsQXaYARrEwybbDGei9mcPNqr", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("3479fbfc3f34e7745860d76137da68f362380c606c"), 21);

	res = ecdsa_address_decode("LWLwtfycqf1uFqypLAug36W4kdgNwrZdNs", decode);
	ck_assert_int_eq(res, 1);
	ck_assert_mem_eq(decode, fromhex("3079fbfc3f34e7745860d76137da68f362380c606c"), 21);

	// invalid char
	res = ecdsa_address_decode("1JwSSubhmg6i000jtyqhUYYH7bZg3Lfy1T", decode);
	ck_assert_int_eq(res, 0);

	// invalid address
	res = ecdsa_address_decode("1111Subhmg6iPtRjtyqhUYYH7bZg3Lfy1T", decode);
	ck_assert_int_eq(res, 0);
}
END_TEST

START_TEST(test_ecdsa_der)
{
	uint8_t sig[64], der[72];
	int res;

	memcpy(sig,      fromhex("9a0b7be0d4ed3146ee262b42202841834698bb3ee39c24e7437df208b8b70771"), 32);
	memcpy(sig + 32, fromhex("2b79ab1e7736219387dffe8d615bbdba87e11477104b867ef47afed1a5ede781"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 71);
	ck_assert_mem_eq(der, fromhex("30450221009a0b7be0d4ed3146ee262b42202841834698bb3ee39c24e7437df208b8b7077102202b79ab1e7736219387dffe8d615bbdba87e11477104b867ef47afed1a5ede781"), 71);

	memcpy(sig,      fromhex("6666666666666666666666666666666666666666666666666666666666666666"), 32);
	memcpy(sig + 32, fromhex("7777777777777777777777777777777777777777777777777777777777777777"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 70);
	ck_assert_mem_eq(der, fromhex("30440220666666666666666666666666666666666666666666666666666666666666666602207777777777777777777777777777777777777777777777777777777777777777"), 70);

	memcpy(sig,      fromhex("6666666666666666666666666666666666666666666666666666666666666666"), 32);
	memcpy(sig + 32, fromhex("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 71);
	ck_assert_mem_eq(der, fromhex("304502206666666666666666666666666666666666666666666666666666666666666666022100eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"), 71);

	memcpy(sig,      fromhex("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"), 32);
	memcpy(sig + 32, fromhex("7777777777777777777777777777777777777777777777777777777777777777"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 71);
	ck_assert_mem_eq(der, fromhex("3045022100eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee02207777777777777777777777777777777777777777777777777777777777777777"), 71);

	memcpy(sig,      fromhex("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"), 32);
	memcpy(sig + 32, fromhex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 72);
	ck_assert_mem_eq(der, fromhex("3046022100eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee022100ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), 72);

	memcpy(sig,      fromhex("0000000000000000000000000000000000000000000000000000000000000066"), 32);
	memcpy(sig + 32, fromhex("0000000000000000000000000000000000000000000000000000000000000077"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 8);
	ck_assert_mem_eq(der, fromhex("3006020166020177"), 8);

	memcpy(sig,      fromhex("0000000000000000000000000000000000000000000000000000000000000066"), 32);
	memcpy(sig + 32, fromhex("00000000000000000000000000000000000000000000000000000000000000ee"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 9);
	ck_assert_mem_eq(der, fromhex("3007020166020200ee"), 9);

	memcpy(sig,      fromhex("00000000000000000000000000000000000000000000000000000000000000ee"), 32);
	memcpy(sig + 32, fromhex("0000000000000000000000000000000000000000000000000000000000000077"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 9);
	ck_assert_mem_eq(der, fromhex("3007020200ee020177"), 9);

	memcpy(sig,      fromhex("00000000000000000000000000000000000000000000000000000000000000ee"), 32);
	memcpy(sig + 32, fromhex("00000000000000000000000000000000000000000000000000000000000000ff"), 32);
	res = ecdsa_sig_to_der(sig, der);
	ck_assert_int_eq(res, 10);
	ck_assert_mem_eq(der, fromhex("3008020200ee020200ff"), 10);
}
END_TEST

static void test_codepoints_curve(const ecdsa_curve *curve) {
	int i, j;
	bignum256 a;
	curve_point p, p1;
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 8; j++) {
			bn_zero(&a);
			a.val[(4*i)/30] = (2*j+1) << (4*i % 30);
			bn_normalize(&a);
			// note that this is not a trivial test.  We add 64 curve
			// points in the table to get that particular curve point.
			scalar_multiply(curve, &a, &p);
			ck_assert_mem_eq(&p, &curve->cp[i][j], sizeof(curve_point));
			bn_zero(&p.y); // test that point_multiply curve, is not a noop
			point_multiply(curve, &a, &curve->G, &p);
			ck_assert_mem_eq(&p, &curve->cp[i][j], sizeof(curve_point));
			// mul 2 test. this should catch bugs
			bn_lshift(&a);
			bn_mod(&a, &curve->order);
			p1 = curve->cp[i][j];
			point_double(curve, &p1);
			// note that this is not a trivial test.  We add 64 curve
			// points in the table to get that particular curve point.
			scalar_multiply(curve, &a, &p);
			ck_assert_mem_eq(&p, &p1, sizeof(curve_point));
			bn_zero(&p.y); // test that point_multiply curve, is not a noop
			point_multiply(curve, &a, &curve->G, &p);
			ck_assert_mem_eq(&p, &p1, sizeof(curve_point));
		}
	}
}

START_TEST(test_codepoints) {
	test_codepoints_curve(&secp256k1);
	test_codepoints_curve(&nist256p1);
}
END_TEST

static void test_mult_border_cases_curve(const ecdsa_curve *curve) {
	bignum256 a;
	curve_point p;
	curve_point expected;
	bn_zero(&a);  // a == 0
	scalar_multiply(curve, &a, &p);
	ck_assert(point_is_infinity(&p));
	point_multiply(curve, &a, &p, &p);
	ck_assert(point_is_infinity(&p));
	point_multiply(curve, &a, &curve->G, &p);
	ck_assert(point_is_infinity(&p));

	bn_addi(&a, 1);  // a == 1
	scalar_multiply(curve, &a, &p);
	ck_assert_mem_eq(&p, &curve->G, sizeof(curve_point));
	point_multiply(curve, &a, &curve->G, &p);
	ck_assert_mem_eq(&p, &curve->G, sizeof(curve_point));

	bn_subtract(&curve->order, &a, &a);  // a == -1
	expected = curve->G;
	bn_subtract(&curve->prime, &expected.y, &expected.y);
	scalar_multiply(curve, &a, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));
	point_multiply(curve, &a, &curve->G, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));

	bn_subtract(&curve->order, &a, &a);
	bn_addi(&a, 1);  // a == 2
	expected = curve->G;
	point_add(curve, &expected, &expected);
	scalar_multiply(curve, &a, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));
	point_multiply(curve, &a, &curve->G, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));

	bn_subtract(&curve->order, &a, &a);  // a == -2
	expected = curve->G;
	point_add(curve, &expected, &expected);
	bn_subtract(&curve->prime, &expected.y, &expected.y);
	scalar_multiply(curve, &a, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));
	point_multiply(curve, &a, &curve->G, &p);
	ck_assert_mem_eq(&p, &expected, sizeof(curve_point));
}

START_TEST(test_mult_border_cases) {
	test_mult_border_cases_curve(&secp256k1);
	test_mult_border_cases_curve(&nist256p1);
}
END_TEST

static void test_scalar_mult_curve(const ecdsa_curve *curve) {
	int i;
	// get two "random" numbers
	bignum256 a = curve->G.x;
	bignum256 b = curve->G.y;
	curve_point p1, p2, p3;
	for (i = 0; i < 1000; i++) {
		/* test distributivity: (a + b)G = aG + bG */
		bn_mod(&a, &curve->order);
		bn_mod(&b, &curve->order);
		scalar_multiply(curve, &a, &p1);
		scalar_multiply(curve, &b, &p2);
		bn_addmod(&a, &b, &curve->order);
		bn_mod(&a, &curve->order);
		scalar_multiply(curve, &a, &p3);
		point_add(curve, &p1, &p2);
		ck_assert_mem_eq(&p2, &p3, sizeof(curve_point));
		// new "random" numbers
		a = p3.x;
		b = p3.y;
	}
}

START_TEST(test_scalar_mult) {
	test_scalar_mult_curve(&secp256k1);
	test_scalar_mult_curve(&nist256p1);
}
END_TEST

static void test_point_mult_curve(const ecdsa_curve *curve) {
	int i;
	// get two "random" numbers and a "random" point
	bignum256 a = curve->G.x;
	bignum256 b = curve->G.y;
	curve_point p = curve->G;
	curve_point p1, p2, p3;
	for (i = 0; i < 200; i++) {
		/* test distributivity: (a + b)P = aP + bP */
		bn_mod(&a, &curve->order);
		bn_mod(&b, &curve->order);
		point_multiply(curve, &a, &p, &p1);
		point_multiply(curve, &b, &p, &p2);
		bn_addmod(&a, &b, &curve->order);
		bn_mod(&a, &curve->order);
		point_multiply(curve, &a, &p, &p3);
		point_add(curve, &p1, &p2);
		ck_assert_mem_eq(&p2, &p3, sizeof(curve_point));
		// new "random" numbers and a "random" point
		a = p1.x;
		b = p1.y;
		p = p3;
	}
}

START_TEST(test_point_mult) {
	test_point_mult_curve(&secp256k1);
	test_point_mult_curve(&nist256p1);
}
END_TEST

static void test_scalar_point_mult_curve(const ecdsa_curve *curve) {
	int i;
	// get two "random" numbers
	bignum256 a = curve->G.x;
	bignum256 b = curve->G.y;
	curve_point p1, p2;
	for (i = 0; i < 200; i++) {
		/* test commutativity and associativity:
		 * a(bG) = (ab)G = b(aG)
		 */
		bn_mod(&a, &curve->order);
		bn_mod(&b, &curve->order);
		scalar_multiply(curve, &a, &p1);
		point_multiply(curve, &b, &p1, &p1);

		scalar_multiply(curve, &b, &p2);
		point_multiply(curve, &a, &p2, &p2);

		ck_assert_mem_eq(&p1, &p2, sizeof(curve_point));

		bn_multiply(&a, &b, &curve->order);
		bn_mod(&b, &curve->order);
		scalar_multiply(curve, &b, &p2);

		ck_assert_mem_eq(&p1, &p2, sizeof(curve_point));

		// new "random" numbers
		a = p1.x;
		b = p1.y;
	}
}

START_TEST(test_scalar_point_mult) {
	test_scalar_point_mult_curve(&secp256k1);
	test_scalar_point_mult_curve(&nist256p1);
}
END_TEST

START_TEST(test_ed25519) {
	// test vectors from https://github.com/torproject/tor/blob/master/src/test/ed25519_vectors.inc
	static const char *vectors[] = {
		"26c76712d89d906e6672dafa614c42e5cb1caac8c6568e4d2493087db51f0d36", // secret
		"c2247870536a192d142d056abefca68d6193158e7c1a59c1654c954eccaff894", // public
		"d23188eac3773a316d46006fa59c095060be8b1a23582a0dd99002a82a0662bd246d8449e172e04c5f46ac0d1404cebe4aabd8a75a1457aa06cae41f3334f104", // selfsig
		"fba7a5366b5cb98c2667a18783f5cf8f4f8d1a2ce939ad22a6e685edde85128d",
		"1519a3b15816a1aafab0b213892026ebf5c0dc232c58b21088d88cb90e9b940d",
		"3a785ac1201c97ee5f6f0d99323960d5f264c7825e61aa7cc81262f15bef75eb4fa5723add9b9d45b12311b6d403eb3ac79ff8e4e631fc3cd51e4ad2185b200b",
		"67e3aa7a14fac8445d15e45e38a523481a69ae35513c9e4143eb1c2196729a0e",
		"081faa81992e360ea22c06af1aba096e7a73f1c665bc8b3e4e531c46455fd1dd",
		"cf431fd0416bfbd20c9d95ef9b723e2acddffb33900edc72195dea95965d52d888d30b7b8a677c0bd8ae1417b1e1a0ec6700deadd5d8b54b6689275e04a04509",
		"d51385942033a76dc17f089a59e6a5a7fe80d9c526ae8ddd8c3a506b99d3d0a6",
		"73cfa1189a723aad7966137cbffa35140bb40d7e16eae4c40b79b5f0360dd65a",
		"2375380cd72d1a6c642aeddff862be8a5804b916acb72c02d9ed052c1561881aa658a5af856fcd6d43113e42f698cd6687c99efeef7f2ce045824440d26c5d00",
		"5c8eac469bb3f1b85bc7cd893f52dc42a9ab66f1b02b5ce6a68e9b175d3bb433",
		"66c1a77104d86461b6f98f73acf3cd229c80624495d2d74d6fda1e940080a96b",
		"2385a472f599ca965bbe4d610e391cdeabeba9c336694b0d6249e551458280be122c2441dd9746a81bbfb9cd619364bab0df37ff4ceb7aefd24469c39d3bc508",
		"eda433d483059b6d1ff8b7cfbd0fe406bfb23722c8f3c8252629284573b61b86",
		"d21c294db0e64cb2d8976625786ede1d9754186ae8197a64d72f68c792eecc19",
		"e500cd0b8cfff35442f88008d894f3a2fa26ef7d3a0ca5714ae0d3e2d40caae58ba7cdf69dd126994dad6be536fcda846d89dd8138d1683cc144c8853dce7607",
		"4377c40431c30883c5fbd9bc92ae48d1ed8a47b81d13806beac5351739b5533d",
		"c4d58b4cf85a348ff3d410dd936fa460c4f18da962c01b1963792b9dcc8a6ea6",
		"d187b9e334b0050154de10bf69b3e4208a584e1a65015ec28b14bcc252cf84b8baa9c94867daa60f2a82d09ba9652d41e8dde292b624afc8d2c26441b95e3c0e",
		"c6bbcce615839756aed2cc78b1de13884dd3618f48367a17597a16c1cd7a290b",
		"95126f14d86494020665face03f2d42ee2b312a85bc729903eb17522954a1c4a",
		"815213640a643d198bd056e02bba74e1c8d2d931643e84497adf3347eb485079c9afe0afce9284cdc084946b561abbb214f1304ca11228ff82702185cf28f60d",
		0,
		0,
		0,
	};
	const char **ssk, **spk, **ssig;
	ssk = vectors;
	spk = vectors + 1;
	ssig = vectors + 2;
	ed25519_public_key pk;
	ed25519_secret_key sk;
	ed25519_signature sig;
	while (*ssk && *spk && *ssig) {
		memcpy(sk, fromhex(*ssk), 32);
		ed25519_publickey(sk, pk);
		ck_assert_mem_eq(pk, fromhex(*spk), 32);
		ed25519_sign(pk, 32, sk, pk, sig);
		ck_assert_mem_eq(sig, fromhex(*ssig), 64);
		ssk += 3;
		spk += 3;
		ssig += 3;
	}
}
END_TEST


// define test suite and cases
Suite *test_suite(void)
{
	Suite *s = suite_create("trezor-crypto");
	TCase *tc;

	tc = tcase_create("base58");
	tcase_add_test(tc, test_base58);
	suite_add_tcase(s, tc);

	tc = tcase_create("bip32");
	tcase_add_test(tc, test_bip32_vector_1);
	tcase_add_test(tc, test_bip32_vector_2);
	tcase_add_test(tc, test_bip32_compare);
	tcase_add_test(tc, test_bip32_cache_1);
	tcase_add_test(tc, test_bip32_cache_2);
	suite_add_tcase(s, tc);

	tc = tcase_create("bip32-nist");
	tcase_add_test(tc, test_bip32_nist_seed);
	tcase_add_test(tc, test_bip32_nist_vector_1);
	tcase_add_test(tc, test_bip32_nist_vector_2);
	tcase_add_test(tc, test_bip32_nist_compare);
	tcase_add_test(tc, test_bip32_nist_repeat);
	suite_add_tcase(s, tc);

	tc = tcase_create("bip32-ed25519");
	tcase_add_test(tc, test_bip32_ed25519_vector_1);
	tcase_add_test(tc, test_bip32_ed25519_vector_2);
	suite_add_tcase(s, tc);
	
	tc = tcase_create("rfc6979");
	tcase_add_test(tc, test_rfc6979);
	suite_add_tcase(s, tc);

	tc = tcase_create("speed");
	tcase_add_test(tc, test_sign_speed);
	tcase_add_test(tc, test_verify_speed);
	suite_add_tcase(s, tc);

	tc = tcase_create("address");
	tcase_add_test(tc, test_address);
	suite_add_tcase(s, tc);

	tc = tcase_create("address_decode");
	tcase_add_test(tc, test_address_decode);
	suite_add_tcase(s, tc);

	tc = tcase_create("wif");
	tcase_add_test(tc, test_wif);
	suite_add_tcase(s, tc);

	tc = tcase_create("ecdsa_der");
	tcase_add_test(tc, test_ecdsa_der);
	suite_add_tcase(s, tc);

	tc = tcase_create("aes");
	tcase_add_test(tc, test_aes);
	suite_add_tcase(s, tc);

	tc = tcase_create("pbkdf2");
	tcase_add_test(tc, test_pbkdf2_hmac_sha256);
	tcase_add_test(tc, test_pbkdf2_hmac_sha512);
	suite_add_tcase(s, tc);

	tc = tcase_create("bip39");
	tcase_add_test(tc, test_mnemonic);
	tcase_add_test(tc, test_mnemonic_check);
	suite_add_tcase(s, tc);

	tc = tcase_create("pubkey_validity");
	tcase_add_test(tc, test_pubkey_validity);
	suite_add_tcase(s, tc);

	tc = tcase_create("codepoints");
	tcase_add_test(tc, test_codepoints);
	suite_add_tcase(s, tc);

	tc = tcase_create("mult_border_cases");
	tcase_add_test(tc, test_mult_border_cases);
	suite_add_tcase(s, tc);

	tc = tcase_create("scalar_mult");
	tcase_add_test(tc, test_scalar_mult);
	suite_add_tcase(s, tc);

	tc = tcase_create("point_mult");
	tcase_add_test(tc, test_point_mult);
	suite_add_tcase(s, tc);

	tc = tcase_create("scalar_point_mult");
	tcase_add_test(tc, test_scalar_point_mult);
	suite_add_tcase(s, tc);

	tc = tcase_create("ed25519");
	tcase_add_test(tc, test_ed25519);
	suite_add_tcase(s, tc);

	return s;
}

// run suite
int main(void)
{
	int number_failed;
	Suite *s = test_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	if (number_failed == 0) {
		printf("PASSED ALL TESTS\n");
	}
	return number_failed;
}
