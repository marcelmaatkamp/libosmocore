/*! \file gprs_bssgp.h */

#pragma once

#include <stdint.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/linuxlist.h>

#include <osmocom/gsm/gsm48.h>
#include <osmocom/gsm/prim.h>

#include <osmocom/gprs/protocol/gsm_08_18.h>
#include <osmocom/gprs/protocol/gsm_24_301.h>
#include <osmocom/gprs/gprs_bssgp_rim.h>

/* gprs_bssgp_util.c */

#define BSSGP_PDUF_UL	0x0001	/* PDU may occur in uplink */
#define BSSGP_PDUF_DL	0x0002	/* PDU may occur in downlink */
#define BSSGP_PDUF_SIG	0x0004	/* PDU may occur on Signaling BVC */
#define BSSGP_PDUF_PTP	0x0008	/* PDU may occur on PTP BVC */
#define BSSGP_PDUF_PTM	0x0010	/* PDU may occur on PTM BVC */

extern const struct osmo_tlv_prot_def osmo_pdef_bssgp;

/*! return the PDU type flags (UL/DL/SIG/PTP/PTM) of specified PDU type */
static inline uint32_t bssgp_pdu_type_flags(uint8_t pdu_type) {
	return osmo_tlv_prot_msgt_flags(&osmo_pdef_bssgp, pdu_type);
}

typedef int (*bssgp_bvc_send)(void *ctx, struct msgb *msg);
extern struct gprs_ns_inst *bssgp_nsi;
void bssgp_set_bssgp_callback(bssgp_bvc_send ns_send, void *data);
struct msgb *bssgp_msgb_alloc(void);
struct msgb *bssgp_msgb_copy(const struct msgb *msg, const char *name);
const char *bssgp_cause_str(enum gprs_bssgp_cause cause);
const char *bssgp_pdu_str(enum bssgp_pdu_type pdu);
int bssgp_tx_bvc_reset_nsei_bvci(uint16_t nsei, uint16_t bvci, enum gprs_bssgp_cause cause, const struct gprs_ra_id *ra_id, uint16_t cell_id);
/* Transmit a simple response such as BLOCK/UNBLOCK/RESET ACK/NACK */
int bssgp_tx_simple_bvci(uint8_t pdu_type, uint16_t nsei,
			 uint16_t bvci, uint16_t ns_bvci);
/* Chapter 10.4.14: Status */
int bssgp_tx_status(uint8_t cause, uint16_t *bvci, struct msgb *orig_msg);

enum bssgp_prim {
	PRIM_BSSGP_DL_UD,
	PRIM_BSSGP_UL_UD,
	PRIM_BSSGP_PTM_UD,

	PRIM_BSSGP_GMM_SUSPEND,
	PRIM_BSSGP_GMM_RESUME,
	PRIM_BSSGP_GMM_PAGING,

	PRIM_NM_FLUSH_LL,
	PRIM_NM_LLC_DISCARDED,
	PRIM_NM_BVC_RESET,
	PRIM_NM_BVC_BLOCK,
	PRIM_NM_BVC_UNBLOCK,
	PRIM_NM_STATUS,

        PRIM_BSSGP_RIM_PDU_TRANSFER,
};

struct osmo_bssgp_prim {
	struct osmo_prim_hdr oph;

	/* common fields */
	uint16_t nsei;
	uint16_t bvci;
	uint32_t tlli;
	struct tlv_parsed *tp;
	struct gprs_ra_id *ra_id;

	/* specific fields */
	union {
		struct {
			uint8_t suspend_ref;
		} resume;
		struct bssgp_ran_information_pdu rim_pdu;
	} u;
};

/* gprs_bssgp.c */

/*! BSSGP flow control (SGSN side) According to Section 8.2 */
struct bssgp_flow_control {
	uint32_t bucket_size_max;	/*!< maximum size of the bucket (octets) */
	uint32_t bucket_leak_rate; 	/*!< leak rate of the bucket (octets/sec) */

	uint32_t bucket_counter;	/*!< number of tokens in the bucket */
	struct timeval time_last_pdu;	/*!< timestamp of last PDU sent */

	/* the built-in queue */
	uint32_t max_queue_depth;	/*!< how many packets to queue (mgs) */
	uint32_t queue_depth;		/*!< current length of queue (msgs) */
	struct llist_head queue;	/*!< linked list of msgb's */
	struct osmo_timer_list timer;	/*!< timer-based dequeueing */

	/*! callback to be called at output of flow control */
	int (*out_cb)(struct bssgp_flow_control *fc, struct msgb *msg,
			uint32_t llc_pdu_len, void *priv);
};

#define BVC_S_BLOCKED	0x0001

/* The per-BTS context that we keep on the SGSN side of the BSSGP link */
struct bssgp_bvc_ctx {
	struct llist_head list;

	struct gprs_ra_id ra_id; /*!< parsed RA ID of the remote BTS */
	uint16_t cell_id; /*!< Cell ID of the remote BTS */

	/* NSEI and BVCI of underlying Gb link.  Together they
	 * uniquely identify a link to a BTS (5.4.4) */
	uint16_t bvci;
	uint16_t nsei;

	uint32_t state;

	struct rate_ctr_group *ctrg;

	struct bssgp_flow_control *fc;
	/*! default maximum size of per-MS bucket in octets */
	uint32_t bmax_default_ms;
	/*! default bucket leak rate of per-MS bucket in octests/s */
	uint32_t r_default_ms;

	/*! BSS or SGSN. This defines the local state. */
	bool is_sgsn;
	/* we might want to add this as a shortcut later, avoiding the NSVC
	 * lookup for every packet, similar to a routing cache */
	//struct gprs_nsvc *nsvc;
};
extern struct llist_head bssgp_bvc_ctxts;
/* Create a BTS Context with BVCI+NSEI */
struct bssgp_bvc_ctx *btsctx_alloc(uint16_t bvci, uint16_t nsei);
/* Find a BTS Context based on parsed RA ID and Cell ID */
struct bssgp_bvc_ctx *btsctx_by_raid_cid(const struct gprs_ra_id *raid, uint16_t cid);
/* Find a BTS context based on BVCI+NSEI tuple */
struct bssgp_bvc_ctx *btsctx_by_bvci_nsei(uint16_t bvci, uint16_t nsei);
/* Free a given BTS context */
void bssgp_bvc_ctx_free(struct bssgp_bvc_ctx *ctx);

#define BVC_F_BLOCKED	0x0001

enum bssgp_ctr {
	BSSGP_CTR_PKTS_IN,
	BSSGP_CTR_PKTS_OUT,
	BSSGP_CTR_BYTES_IN,
	BSSGP_CTR_BYTES_OUT,
	BSSGP_CTR_BLOCKED,
	BSSGP_CTR_DISCARDED,
	BSSGP_CTR_STATUS,
};


#include <osmocom/gsm/tlv.h>
#include <osmocom/gprs/gprs_msgb.h>

/* BSSGP-UL-UNITDATA.ind */
int bssgp_rcvmsg(struct msgb *msg);

/* BSSGP-DL-UNITDATA.req */
struct bssgp_lv {
	uint16_t len;
	uint8_t *v;
};
/* parameters for BSSGP downlink userdata transmission */
struct bssgp_dl_ud_par {
	uint32_t *tlli;
	char *imsi;
	struct bssgp_flow_control *fc;
	uint16_t drx_parms;
	/* FIXME: priority */
	struct bssgp_lv ms_ra_cap;
	uint8_t qos_profile[3];
};
int bssgp_tx_dl_ud(struct msgb *msg, uint16_t pdu_lifetime,
		   struct bssgp_dl_ud_par *dup);

uint16_t bssgp_parse_cell_id(struct gprs_ra_id *raid, const uint8_t *buf);
int bssgp_create_cell_id(uint8_t *buf, const struct gprs_ra_id *raid,
			 uint16_t cid);

int bssgp_parse_cell_id2(struct osmo_routing_area_id *raid, uint16_t *cid,
			 const uint8_t *buf, size_t buf_len);
int bssgp_create_cell_id2(uint8_t *buf, size_t buf_len,
			  const struct osmo_routing_area_id *raid,
			  uint16_t cid);

/* Wrapper around TLV parser to parse BSSGP IEs */
static inline int bssgp_tlv_parse(struct tlv_parsed *tp, const uint8_t *buf, int len)
{
	return tlv_parse(tp, &tvlv_att_def, buf, len, 0, 0);
}

/*! BSSGP Paging mode */
enum bssgp_paging_mode {
	BSSGP_PAGING_PS,
	BSSGP_PAGING_CS,
};

/*! BSSGP Paging scope */
enum bssgp_paging_scope {
	BSSGP_PAGING_BSS_AREA,		/*!< all cells in BSS */
	BSSGP_PAGING_LOCATION_AREA,	/*!< all cells in LA */
	BSSGP_PAGING_ROUTEING_AREA,	/*!< all cells in RA */
	BSSGP_PAGING_BVCI,		/*!< one cell */
};

/*! BSSGP paging information */
struct bssgp_paging_info {
	enum bssgp_paging_mode mode;	/*!< CS or PS paging */
	enum bssgp_paging_scope scope;	/*!< bssgp_paging_scope */
	struct gprs_ra_id raid;		/*!< RA Identifier */
	uint16_t bvci;			/*!< BVCI */
	char *imsi;			/*!< IMSI, if any */
	uint32_t *ptmsi;		/*!< P-TMSI, if any */
	uint16_t drx_params;		/*!< DRX parameters */
	uint8_t qos[3];			/*!< QoS parameters */
};

/* Send a single GMM-PAGING.req to a given NSEI/NS-BVCI */
int bssgp_tx_paging(uint16_t nsei, uint16_t ns_bvci,
		    struct bssgp_paging_info *pinfo);

void bssgp_fc_init(struct bssgp_flow_control *fc,
		   uint32_t bucket_size_max, uint32_t bucket_leak_rate,
		   uint32_t max_queue_depth,
		   int (*out_cb)(struct bssgp_flow_control *fc, struct msgb *msg,
				 uint32_t llc_pdu_len, void *priv));

/* input function of the flow control implementation, called first
 * for the MM flow control, and then as the MM flow control output
 * callback in order to perform BVC flow control */
int bssgp_fc_in(struct bssgp_flow_control *fc, struct msgb *msg,
		uint32_t llc_pdu_len, void *priv);

/* Initialize the Flow Control parameters for a new MS according to
 * default values for the BVC specified by BVCI and NSEI */
int bssgp_fc_ms_init(struct bssgp_flow_control *fc_ms, uint16_t bvci,
		     uint16_t nsei, uint32_t max_queue_depth);

void bssgp_flush_all_queues(void);
void bssgp_fc_flush_queue(struct bssgp_flow_control *fc);

/* gprs_bssgp_vty.c */
int bssgp_vty_init(void);
void bssgp_set_log_ss(int ss) OSMO_DEPRECATED("Use DLBSSGP instead!\n");

int bssgp_prim_cb(struct osmo_prim_hdr *oph, void *ctx);
