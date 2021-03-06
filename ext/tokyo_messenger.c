#include "tokyo_messenger.h"

extern VALUE StringRaw(const char *buf, int bsiz){
  VALUE vval;
  int i;
  vval = rb_str_buf_new2("");
  char s[5];

  for(i=0;i<bsiz;i++){
    char c = *buf++;
    s[0] = c;
    rb_str_buf_cat(vval, s, 1);
  }
//    buf -= bsiz;
//    rb_str_buf_cat2(vval, "");
  return vval;
}

extern VALUE StringValueEx(VALUE vobj){
  if (rb_respond_to(vobj, rb_intern("to_tokyo_tyrant"))) {
    return rb_convert_type(vobj, T_STRING, "String", "to_tokyo_tyrant");
  } else if (rb_respond_to(vobj, rb_intern("to_s"))) {
    return rb_convert_type(vobj, T_STRING, "String", "to_s");
  } else {
    rb_raise(rb_eArgError, "can't stringify object");
  }
  return StringValue(vobj);
}

extern TCLIST *varytolist(VALUE vary){
  VALUE vval;
  TCLIST *list;
  int i, num;
  num = RARRAY_LEN(vary);
  list = tclistnew2(num);
  for(i = 0; i < num; i++){
    vval = rb_ary_entry(vary, i);
    vval = StringValueEx(vval);
    tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return list;
}

extern VALUE listtovary(TCLIST *list){
  VALUE vary;
  const char *vbuf;
  int i, num, vsiz;
  num = tclistnum(list);
  vary = rb_ary_new2(num);
  for(i = 0; i < num; i++){
    vbuf = tclistval(list, i, &vsiz);
    rb_ary_push(vary, rb_str_new(vbuf, vsiz));
  }
  return vary;
}

extern TCMAP *vhashtomap(VALUE vhash){
  VALUE vkeys, vkey, vval;
  TCMAP *map;
  int i, num;
  map = tcmapnew2(31);
  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);
    vkey = StringValueEx(vkey);
    vval = StringValueEx(vval);
    tcmapput(map, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return map;
}

extern VALUE maptovhash(TCMAP *map){
  const char *kbuf;
  int ksiz, vsiz;
  VALUE vhash;
  vhash = rb_hash_new();
  tcmapiterinit(map);
  while((kbuf = tcmapiternext(map, &ksiz)) != NULL){
    const char *vbuf = tcmapiterval(kbuf, &vsiz);
    rb_hash_aset(vhash, rb_str_new(kbuf, ksiz), StringRaw(vbuf, vsiz));
  }
  return vhash;
}

extern TCMAP *varytomap(VALUE vary){
  int i;
  TCLIST *keys;
  TCMAP *recs = tcmapnew();
  keys = varytolist(vary);
  for(i = 0; i < tclistnum(keys); i++){
    int ksiz;
    const char *kbuf = tclistval(keys, i, &ksiz);
    tcmapput(recs, kbuf, ksiz, "", 0);
  }
  tclistdel(keys);
  return recs;
}

extern TCLIST *vhashtolist(VALUE vhash){
  /*
  Seems like something like this might work just as well
  vary = rb_hash_to_a(vhash);
  vary = rb_ary_flatten(vary);
  args = varytolist(vary);
  */

  VALUE vkeys, vkey, vval;
  TCLIST *list;
  int i, num;
  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  list = tclistnew2(num);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);
    vkey = StringValueEx(vkey);
    vval = StringValueEx(vval);
    tclistpush(list, RSTRING_PTR(vkey), RSTRING_LEN(vkey));
    tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return list;
}

VALUE mTokyoMessenger;
VALUE eTokyoMessengerError;
VALUE cDB;
VALUE cTable;
VALUE cQuery;

void Init_tokyo_messenger(){
  mTokyoMessenger = rb_define_module("TokyoMessenger");
  eTokyoMessengerError = rb_define_class("TokyoMessengerError", rb_eStandardError);
  init_mod();

  cDB = rb_define_class_under(mTokyoMessenger, "DB", rb_cObject);
  rb_include_module(cDB, mTokyoMessenger);
  init_db();

  cTable = rb_define_class_under(mTokyoMessenger, "Table", rb_cObject);
  rb_include_module(cTable, mTokyoMessenger);
  init_table();

  cQuery = rb_define_class_under(mTokyoMessenger, "Query", rb_cObject);
  init_query();
}
