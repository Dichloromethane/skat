
class Event {
    OID oid;
	const Action& cause;
  public:
    Event(OID oid, const Action& cause): oid(oid), cause(cause) {}
	virtual Event(void *ser)=0; 
    virtual void *serializeAs(Player p)=0;
	virtual ClientGameState apply(ClientGameState cgs)=0;
};
