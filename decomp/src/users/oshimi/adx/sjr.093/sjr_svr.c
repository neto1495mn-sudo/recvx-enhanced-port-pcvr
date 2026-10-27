// *****************************************************************************
// FILE -- /home/oshimi/users/oshimi/adx/sjr.093/sjr_svr.c
// *****************************************************************************

/* 00013ad0 00000078 */ void sjrmt_rbf_create(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJ sj;
}

/* 00013b48 00000068 */ void sjrmt_mem_create(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJ sj;
}

/* 00013bb0 00000078 */ void sjrmt_uni_create(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJ sj;
}

/* 00013c28 0000005c */ void sjrmt_destroy(/* 0x18(sp) */ void *obj, /* 0x1c(sp) */ Sint32 *arg, /* 0x20(sp) */ Sint32 narg) {}

/* 00013c84 000000f0 */ void sjrmt_get_uuid(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ UUID *uuid;
	/* 0x14(sp) */ Sint32 *tmp;
}

/* 00013d74 0000005c */ void sjrmt_reset(/* 0x18(sp) */ void *obj, /* 0x1c(sp) */ Sint32 *arg, /* 0x20(sp) */ Sint32 narg) {}

/* 00013dd0 000000a8 */ void sjrmt_get_chunk(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJCK ck;
}

/* 00013e78 000000a0 */ void sjrmt_unget_chunk(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJCK ck;
}

/* 00013f18 000000a0 */ void sjrmt_put_chunk(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ SJCK ck;
}

/* 00013fb8 00000080 */ void sjrmt_get_num_data(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ Sint32 nbyte;
}

/* 00014038 000000ac */ void sjrmt_is_get_chunk(/* 0x20(sp) */ void *obj, /* 0x24(sp) */ Sint32 *arg, /* 0x28(sp) */ Sint32 narg) {
	/* 0x10(sp) */ Sint32 ret;
	/* 0x14(sp) */ Sint32 rbyte;
}

/* 000140e4 00000164 */ void SJRMT_InitSvr() {}
/* 00014248 0000004c */ void SJRMT_FinishSvr() {}