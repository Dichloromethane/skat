

class DrueckeSkatAction : public virtual Action {
}

class DrueckeSkatEvent : public virtual Event {
	card c1, c2;

  public:
    DrueckeSkatEvent(OID oid, Action& cause, card c1, card c2): 
	  Event(oid, cause), c1(c1), c2(c2) {}
    
	DrueckeSkatEvent(void *buf) {
	}

    void *serializeAs(Player p) override {
	  if (this->cause.by(p))
		serialize_cards()
	  else
		serialize_player();
	}
}
