#include "skat/util.h"
#include <skat/client.h>
#include <skat/package.h>
#include <string.h>

#include "skat/package.h"

#undef PACKAGE_C_HDR
#define PACKAGE_HDR_TO_STRING

#include "skat/package.h"

static void
read_payload_error(payload_error *p, byte_buf *bb) {
  p->type = (conn_error_type) byte_buf_read_i8(bb);
}

static void
write_payload_error(const payload_error *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->type);
}

static void
free_payload_error(payload_error *p) {}

static void
read_payload_join(payload_join *p, byte_buf *bb) {
  p->network_protocol_version = byte_buf_read_u16(bb);
  p->name = byte_buf_read_str(bb);
}

static void
write_payload_join(const payload_join *p, byte_buf *bb) {
  byte_buf_write_u16(bb, p->network_protocol_version);
  byte_buf_write_str(bb, p->name);
}

static void
free_payload_join(payload_join *p) {
  if (p->name) {
	free(p->name);
	p->name = NULL;
  }
}

static void
read_payload_confirm_join(payload_confirm_join *p, byte_buf *bb) {
  p->gupid = byte_buf_read_i8(bb);
}

static void
write_payload_confirm_join(const payload_confirm_join *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->gupid);
}

static void
free_payload_confirm_join(payload_confirm_join *p) {}

static void
read_payload_resume(payload_resume *p, byte_buf *bb) {
  p->network_protocol_version = byte_buf_read_u16(bb);
  p->name = byte_buf_read_str(bb);
}

static void
write_payload_resume(const payload_resume *p, byte_buf *bb) {
  byte_buf_write_u16(bb, p->network_protocol_version);
  byte_buf_write_str(bb, p->name);
}

static void
free_payload_resume(payload_resume *p) {
  if (p->name) {
	free(p->name);
	p->name = NULL;
  }
}

static void
read_payload_confirm_resume(payload_confirm_resume *p, byte_buf *bb) {
  p->gupid = byte_buf_read_i8(bb);
}

static void
write_payload_confirm_resume(const payload_confirm_resume *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->gupid);
}

static void
free_payload_confirm_resume(payload_confirm_resume *p) {}

static void
read_payload_resync(payload_resync *p, byte_buf *bb) {
  p->scs.sgs.cgphase = (game_phase) byte_buf_read_i8(bb);

  p->scs.sgs.rs.rphase = (reiz_phase) byte_buf_read_i8(bb);
  p->scs.sgs.rs.waiting_teller = byte_buf_read_bool(bb);
  p->scs.sgs.rs.reizwert = byte_buf_read_u16(bb);
  p->scs.sgs.rs.winner = byte_buf_read_i8(bb);

  read_game_rules(&p->scs.sgs.gr, bb);

  for (int ap = 0; ap < 3; ap++)
	p->scs.sgs.active_players[ap] = byte_buf_read_i8(bb);
  for (int gupid = 0; gupid < 4; gupid++)
	p->scs.sgs.score[gupid] = byte_buf_read_i64(bb);

  read_stich(&p->scs.sgs.curr_stich, bb);
  read_stich(&p->scs.sgs.last_stich, bb);

  p->scs.sgs.stich_num = byte_buf_read_u8(bb);
  p->scs.sgs.alleinspieler = byte_buf_read_i8(bb);
  p->scs.sgs.took_skat = byte_buf_read_bool(bb);

  p->scs.my_hand = byte_buf_read_u32(bb);
  p->scs.my_gupid = byte_buf_read_i8(bb);
  p->scs.my_active_player_index = byte_buf_read_i8(bb);
  p->scs.my_partner = byte_buf_read_i8(bb);
  p->scs.ist_alleinspieler = byte_buf_read_bool(bb);

  for (int gupid = 0; gupid < 4; gupid++) {
	if (byte_buf_read_bool(bb)) {
	  p->aps[gupid] = byte_buf_read_i8(bb);
	  p->player_names[gupid] = byte_buf_read_str(bb);
	} else {
	  p->aps[gupid] = -1;
	  p->player_names[gupid] = NULL;
	}
  }
}

static void
write_payload_resync(const payload_resync *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->scs.sgs.cgphase);

  byte_buf_write_i8(bb, p->scs.sgs.rs.rphase);
  byte_buf_write_bool(bb, p->scs.sgs.rs.waiting_teller);
  byte_buf_write_u16(bb, p->scs.sgs.rs.reizwert);
  byte_buf_write_i8(bb, p->scs.sgs.rs.winner);

  write_game_rules(&p->scs.sgs.gr, bb);

  for (int ap = 0; ap < 3; ap++)
	byte_buf_write_i8(bb, p->scs.sgs.active_players[ap]);
  for (int gupid = 0; gupid < 4; gupid++)
	byte_buf_write_i64(bb, p->scs.sgs.score[gupid]);

  write_stich(&p->scs.sgs.curr_stich, bb);
  write_stich(&p->scs.sgs.last_stich, bb);

  byte_buf_write_u8(bb, p->scs.sgs.stich_num);
  byte_buf_write_i8(bb, p->scs.sgs.alleinspieler);
  byte_buf_write_bool(bb, p->scs.sgs.took_skat);

  byte_buf_write_u32(bb, p->scs.my_hand);
  byte_buf_write_i8(bb, p->scs.my_gupid);
  byte_buf_write_i8(bb, p->scs.my_active_player_index);
  byte_buf_write_i8(bb, p->scs.my_partner);
  byte_buf_write_bool(bb, p->scs.ist_alleinspieler);

  for (int gupid = 0; gupid < 4; gupid++) {
	if (p->player_names[gupid] != NULL) {
	  byte_buf_write_bool(bb, true);
	  byte_buf_write_i8(bb, p->aps[gupid]);
	  byte_buf_write_str(bb, p->player_names[gupid]);
	} else {
	  byte_buf_write_bool(bb, false);
	}
  }
}

static void
free_payload_resync(payload_resync *p) {
  for (int gupid = 0; gupid < 4; gupid++) {
	char *name = p->player_names[gupid];
	if (name) {
	  free(name);
	  p->player_names[gupid] = NULL;
	}
  }
}

static void
read_payload_notify_join(payload_notify_join *p, byte_buf *bb) {
  p->gupid = byte_buf_read_i8(bb);
  p->ap = byte_buf_read_i8(bb);
  p->name = byte_buf_read_str(bb);
}

static void
write_payload_notify_join(const payload_notify_join *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->gupid);
  byte_buf_write_i8(bb, p->ap);
  byte_buf_write_str(bb, p->name);
}

static void
free_payload_notify_join(payload_notify_join *p) {
  if (p->name) {
	free(p->name);
	p->name = NULL;
  }
}

static void
read_payload_notify_leave(payload_notify_leave *p, byte_buf *bb) {
  p->gupid = byte_buf_read_i8(bb);
}

static void
write_payload_notify_leave(const payload_notify_leave *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->gupid);
}

static void
free_payload_notify_leave(payload_notify_leave *p) {}

static void
read_payload_action(payload_action *p, byte_buf *bb) {
  p->ac.type = (action_type) byte_buf_read_i8(bb);
  p->ac.id = byte_buf_read_i64(bb);

  switch (p->ac.type) {
	case ACTION_PLAY_CARD:
	  p->ac.card = byte_buf_read_i8(bb);
	  break;
	case ACTION_REIZEN_NUMBER:
	  p->ac.reizwert = byte_buf_read_u16(bb);
	  break;
	case ACTION_SKAT_PRESS:
	  for (int i = 0; i < 2; i++)
		p->ac.skat_press_cards[i] = byte_buf_read_i8(bb);
	  break;
	case ACTION_CALL_GAME:
	  read_game_rules(&p->ac.gr, bb);
	  break;
	case ACTION_READY:
	case ACTION_REIZEN_CONFIRM:
	case ACTION_REIZEN_PASSE:
	case ACTION_SKAT_TAKE:
	case ACTION_SKAT_LEAVE:
	  break;
	default:
	  DERROR_PRINTF("Cannot read action from byte buf: unknown type %s",
					action_name_table[p->ac.type]);
	  break;
  }
}

static void
write_payload_action(const payload_action *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->ac.type);
  byte_buf_write_i64(bb, p->ac.id);

  switch (p->ac.type) {
	case ACTION_PLAY_CARD:
	  byte_buf_write_i8(bb, p->ac.card);
	  break;
	case ACTION_REIZEN_NUMBER:
	  byte_buf_write_u16(bb, p->ac.reizwert);
	  break;
	case ACTION_SKAT_PRESS:
	  for (int i = 0; i < 2; i++)
		byte_buf_write_i8(bb, p->ac.skat_press_cards[i]);
	  break;
	case ACTION_CALL_GAME:
	  write_game_rules(&p->ac.gr, bb);
	  break;
	case ACTION_READY:
	case ACTION_REIZEN_CONFIRM:
	case ACTION_REIZEN_PASSE:
	case ACTION_SKAT_TAKE:
	case ACTION_SKAT_LEAVE:
	  break;
	default:
	  DERROR_PRINTF("Cannot write action to byte buf: unknown type %s",
					action_name_table[p->ac.type]);
	  break;
  }
}

static void
free_payload_action(payload_action *p) {}

static void
read_payload_event(payload_event *p, byte_buf *bb) {
  p->ev.type = (event_type) byte_buf_read_i8(bb);
  p->ev.answer_to = byte_buf_read_i64(bb);
  p->ev.acting_player = byte_buf_read_i8(bb);

  switch (p->ev.type) {
	case EVENT_START_ROUND:
	  for (int ap = 0; ap < 3; ap++)
		p->ev.current_active_players[ap] = byte_buf_read_i8(bb);
	  break;
	case EVENT_DISTRIBUTE_CARDS:
	  p->ev.hand = byte_buf_read_u32(bb);
	  break;
	case EVENT_REIZEN_NUMBER:
	  p->ev.reizwert = byte_buf_read_u16(bb);
	  break;
	case EVENT_REIZEN_DONE:
	  p->ev.alleinspieler = byte_buf_read_i8(bb);
	  p->ev.reizwert_final = byte_buf_read_u16(bb);
	  break;
	case EVENT_SKAT_TAKE:
	  for (int i = 0; i < 2; i++)
		p->ev.skat[i] = byte_buf_read_i8(bb);
	  break;
	case EVENT_SKAT_PRESS:
	  for (int i = 0; i < 2; i++)
		p->ev.skat_press_cards[i] = byte_buf_read_i8(bb);
	  break;
	case EVENT_PLAY_CARD:
	  p->ev.card = byte_buf_read_i8(bb);
	  break;
	case EVENT_STICH_DONE:
	  p->ev.stich_winner = byte_buf_read_i8(bb);
	  break;
	case EVENT_ROUND_DONE:
	  for (int gupid = 0; gupid < 4; gupid++)
		p->ev.score_total[gupid] = byte_buf_read_i64(bb);
	  break;
	case EVENT_GAME_CALLED:
	  read_game_rules(&p->ev.gr, bb);
	  break;
	case EVENT_ANNOUNCE_SCORES:
	  p->ev.rr.round_winner = byte_buf_read_i8(bb);
	  for (int i = 0; i < 3; i++)
		p->ev.rr.player_points[i] = byte_buf_read_u8(bb);
	  for (int i = 0; i < 3; i++)
		p->ev.rr.player_stich_card_count[i] = byte_buf_read_u8(bb);
	  for (int i = 0; i < 3; i++)
		p->ev.rr.round_score[i] = byte_buf_read_i64(bb);
	  p->ev.rr.spielwert = byte_buf_read_u16(bb);
	  p->ev.rr.loss_type = (unsigned int) byte_buf_read_i8(bb);
	  p->ev.rr.normal_end = byte_buf_read_bool(bb);
	  p->ev.rr.schneider = byte_buf_read_bool(bb);
	  p->ev.rr.schwarz = byte_buf_read_bool(bb);
	  break;
	case EVENT_ILLEGAL_ACTION:
	case EVENT_START_GAME:
	case EVENT_REIZEN_CONFIRM:
	case EVENT_REIZEN_PASSE:
	case EVENT_SKAT_LEAVE:
	  break;
	default:
	  DERROR_PRINTF("Cannot read event from byte buf: unknown type %s",
					event_name_table[p->ev.type]);
	  break;
	case EVENT_INVALID:
	  break;
  }
}

static void
write_payload_event(const payload_event *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->ev.type);
  byte_buf_write_i64(bb, p->ev.answer_to);
  byte_buf_write_i8(bb, p->ev.acting_player);

  switch (p->ev.type) {
	case EVENT_START_ROUND:
	  for (int ap = 0; ap < 3; ap++)
		byte_buf_write_i8(bb, p->ev.current_active_players[ap]);
	  break;
	case EVENT_DISTRIBUTE_CARDS:
	  byte_buf_write_u32(bb, p->ev.hand);
	  break;
	case EVENT_REIZEN_NUMBER:
	  byte_buf_write_u16(bb, p->ev.reizwert);
	  break;
	case EVENT_REIZEN_DONE:
	  byte_buf_write_i8(bb, p->ev.alleinspieler);
	  byte_buf_write_u16(bb, p->ev.reizwert_final);
	  break;
	case EVENT_SKAT_TAKE:
	  for (int i = 0; i < 2; i++)
		byte_buf_write_i8(bb, p->ev.skat[i]);
	  break;
	case EVENT_SKAT_PRESS:
	  for (int i = 0; i < 2; i++)
		byte_buf_write_i8(bb, p->ev.skat_press_cards[i]);
	  break;
	case EVENT_PLAY_CARD:
	  byte_buf_write_i8(bb, p->ev.card);
	  break;
	case EVENT_STICH_DONE:
	  byte_buf_write_i8(bb, p->ev.stich_winner);
	  break;
	case EVENT_ROUND_DONE:
	  for (int gupid = 0; gupid < 4; gupid++)
		byte_buf_write_i64(bb, p->ev.score_total[gupid]);
	  break;
	case EVENT_GAME_CALLED:
	  write_game_rules(&p->ev.gr, bb);
	  break;
	case EVENT_ANNOUNCE_SCORES:
	  byte_buf_write_i8(bb, p->ev.rr.round_winner);
	  for (int i = 0; i < 3; i++)
		byte_buf_write_u8(bb, p->ev.rr.player_points[i]);
	  for (int i = 0; i < 3; i++)
		byte_buf_write_u8(bb, p->ev.rr.player_stich_card_count[i]);
	  for (int i = 0; i < 3; i++)
		byte_buf_write_i64(bb, p->ev.rr.player_stich_card_count[i]);
	  byte_buf_write_u16(bb, p->ev.rr.spielwert);
	  byte_buf_write_i8(bb, p->ev.rr.loss_type);
	  byte_buf_write_bool(bb, p->ev.rr.normal_end);
	  byte_buf_write_bool(bb, p->ev.rr.schneider);
	  byte_buf_write_bool(bb, p->ev.rr.schwarz);
	  break;
	case EVENT_ILLEGAL_ACTION:
	case EVENT_START_GAME:
	case EVENT_REIZEN_CONFIRM:
	case EVENT_REIZEN_PASSE:
	case EVENT_SKAT_LEAVE:
	  break;
	default:
	  DERROR_PRINTF("Cannot write event to byte buf: unknown type %s",
					event_name_table[p->ev.type]);
	  break;
	case EVENT_INVALID:
	  break;
  }
}

static void
free_payload_event(payload_event *p) {}

void
package_read(package *p, byte_buf *bb) {
  p->type = (package_type) byte_buf_read_i8(bb);

  switch (p->type) {
	case PACKAGE_ERROR:
	  read_payload_error(&p->pl_er, bb);
	  break;
	case PACKAGE_JOIN:
	  read_payload_join(&p->pl_j, bb);
	  break;
	case PACKAGE_CONFIRM_JOIN:
	  read_payload_confirm_join(&p->pl_cj, bb);
	  break;
	case PACKAGE_RESUME:
	  read_payload_resume(&p->pl_rm, bb);
	  break;
	case PACKAGE_CONFIRM_RESUME:
	  read_payload_confirm_resume(&p->pl_cr, bb);
	  break;
	case PACKAGE_RESYNC:
	  read_payload_resync(&p->pl_rs, bb);
	  break;
	case PACKAGE_NOTIFY_JOIN:
	  read_payload_notify_join(&p->pl_nj, bb);
	  break;
	case PACKAGE_NOTIFY_LEAVE:
	  read_payload_notify_leave(&p->pl_nl, bb);
	  break;
	case PACKAGE_ACTION:
	  read_payload_action(&p->pl_a, bb);
	  break;
	case PACKAGE_EVENT:
	  read_payload_event(&p->pl_ev, bb);
	  break;
	case PACKAGE_REQUEST_RESYNC:
	case PACKAGE_CONFIRM_RESYNC:
	case PACKAGE_DISCONNECT:
	  break;
	default:
	  DERROR_PRINTF("Cannot read package from byte buf: unknown type %s",
					package_name_table[p->type]);
	  break;
  }
}

void
package_write(const package *p, byte_buf *bb) {
  byte_buf_write_i8(bb, p->type);

  switch (p->type) {
	case PACKAGE_ERROR:
	  write_payload_error(&p->pl_er, bb);
	  break;
	case PACKAGE_JOIN:
	  write_payload_join(&p->pl_j, bb);
	  break;
	case PACKAGE_CONFIRM_JOIN:
	  write_payload_confirm_join(&p->pl_cj, bb);
	  break;
	case PACKAGE_RESUME:
	  write_payload_resume(&p->pl_rm, bb);
	  break;
	case PACKAGE_CONFIRM_RESUME:
	  write_payload_confirm_resume(&p->pl_cr, bb);
	  break;
	case PACKAGE_RESYNC:
	  write_payload_resync(&p->pl_rs, bb);
	  break;
	case PACKAGE_NOTIFY_JOIN:
	  write_payload_notify_join(&p->pl_nj, bb);
	  break;
	case PACKAGE_NOTIFY_LEAVE:
	  write_payload_notify_leave(&p->pl_nl, bb);
	  break;
	case PACKAGE_ACTION:
	  write_payload_action(&p->pl_a, bb);
	  break;
	case PACKAGE_EVENT:
	  write_payload_event(&p->pl_ev, bb);
	  break;
	case PACKAGE_REQUEST_RESYNC:
	case PACKAGE_CONFIRM_RESYNC:
	case PACKAGE_DISCONNECT:
	  break;
	default:
	  DERROR_PRINTF("Cannot write package to byte buf: unknown type %s",
					package_name_table[p->type]);
	  break;
  }
}

void
package_clean(package *p) {
  memset(p, '\0', sizeof(package));
  p->type = PACKAGE_INVALID;
}

void
package_free(package *p) {
  switch (p->type) {
	case PACKAGE_ERROR:
	  free_payload_error(&p->pl_er);
	  break;
	case PACKAGE_JOIN:
	  free_payload_join(&p->pl_j);
	  break;
	case PACKAGE_CONFIRM_JOIN:
	  free_payload_confirm_join(&p->pl_cj);
	  break;
	case PACKAGE_RESUME:
	  free_payload_resume(&p->pl_rm);
	  break;
	case PACKAGE_CONFIRM_RESUME:
	  free_payload_confirm_resume(&p->pl_cr);
	  break;
	case PACKAGE_RESYNC:
	  free_payload_resync(&p->pl_rs);
	  break;
	case PACKAGE_NOTIFY_JOIN:
	  free_payload_notify_join(&p->pl_nj);
	  break;
	case PACKAGE_NOTIFY_LEAVE:
	  free_payload_notify_leave(&p->pl_nl);
	  break;
	case PACKAGE_ACTION:
	  free_payload_action(&p->pl_a);
	  break;
	case PACKAGE_EVENT:
	  free_payload_event(&p->pl_ev);
	  break;
	case PACKAGE_REQUEST_RESYNC:
	case PACKAGE_CONFIRM_RESYNC:
	case PACKAGE_DISCONNECT:
	  break;
	default:
	  DERROR_PRINTF("Cannot free package: unknown type %s",
					package_name_table[p->type]);
	  break;
  }
  package_clean(p);
}
