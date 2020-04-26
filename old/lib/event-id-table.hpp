


namespace TypeIDTable {

  template<int Cond, typename T1, typename T2>
  struct if_str {
	using T = T1;
  };

  template<typename T1, typename T2>
  struct if_str<0, T1, T2> {
	using T = T2;
  };

  template<int Cond, typename T1, typename T2>
  using if_t = if_str<Cond, T1, T2>::T;



  template<typename T, typename S>
  struct is_same {
	constexpr int val = 0;
  };

  template<typename T>
  struct is_same<T, T> {
	constexpr int val = 1;
  };

  template<typename T, typename S>
  constexpr int is_same_v = is_same<T, S>::val;



  template<typename T, typename... Types>
  struct Entry {
    using T = T;
	using Next = Entry<Types...>;
    constexpr int ID = Next::ID + 1;
	template<typename A>
	using add = Entry<A, T, Types...>;
  };
  
  template<>
  struct Entry {
    using T = void;
    constexpr int ID = 0;
	template<typename E>
	using add = Entry<E::T, E::Types...>;
  };
  
  template<typename E, typename T>
  struct findT {
    using Res = if_t<is_same_v<E::T, T>, E, find<E::Next>>;
	constexpr int ID = Res::ID;
  };

  template<typename E, int ID>
  struct findID {
    using Res = if_t<E::ID == ID, E, find<E::Next>>;
	using T = Res::T;
  };



  template<typename E>
  struct selectT {
	template<typename Selector, typename... Args>
    void *res(Selector s, Args... args) {
	  if (s(E::id))
		return (void *) new T{args...};
	  else
		return (selectT<E::Next>{}).res(s, args...);
	}
  };

  template<>
  struct selectT<void> {
	template<typename Selector, typename... Args>
    void *res(Selector s, Args... args) {
	  return NULL;
	}
  };



  template<typename E>
  struct findTdyn_type {
    template<typename... ConstrArgs>
    void *find(int id, ConstrArgs... args) {
	  return (selectT<E>{}).template res<ConstrArgs...>([=](int idc) { return idc == id; }, args...);
	}
  };

  template<typename E> 
  constexpr findTdyn_type findTdyn = findTdyn_type<E>{};
}

using EventTable = TypeIDTable<E1, E2, E3>;
using EventTable = EventTable::add<E4>;
// findTdyn<EventTable>.template find<void *>(id, ser_data)
// findT<EventTable, E2>::ID;
// findID<EventTable, 1>::T;
