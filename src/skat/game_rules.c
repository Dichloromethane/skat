#include "skat/game_rules.h"

void
read_game_rules(game_rules *this, byte_buf *bb) {
  this->type = (game_type) byte_buf_read_i8(bb);
  this->trumpf = (card_color) byte_buf_read_i8(bb);
  this->hand = byte_buf_read_bool(bb);
  this->schneider_angesagt = byte_buf_read_bool(bb);
  this->schwarz_angesagt = byte_buf_read_bool(bb);
  this->ouvert = byte_buf_read_bool(bb);
}

void
write_game_rules(const game_rules *this, byte_buf *bb) {
  byte_buf_write_i8(bb, this->type);
  byte_buf_write_i8(bb, this->trumpf);
  byte_buf_write_bool(bb, this->hand);
  byte_buf_write_bool(bb, this->schneider_angesagt);
  byte_buf_write_bool(bb, this->schwarz_angesagt);
  byte_buf_write_bool(bb, this->ouvert);
}
