#include "tokyo_messenger_db.h"

extern TCRDB *mTokyoMessenger_getdb(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  return db;
}

extern void mTokyoMessenger_exception(VALUE vself){
  int ecode;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  ecode = tcrdbecode(db);
  rb_raise(eTokyoMessengerError, tcrdberrmsg(ecode));
}

static void mTokyoMessenger_free(TCRDB *db){
  tcrdbdel(db);
}

static VALUE mTokyoMessenger_server(VALUE vself){
  return rb_iv_get(vself, "@server");
}

static VALUE mTokyoMessenger_close(VALUE vself){
  int ecode;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  if(!tcrdbclose(db)){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoMessengerError, "close error: %s", tcrdberrmsg(ecode));
  }
  return Qtrue;
}

static VALUE mTokyoMessenger_connect(VALUE vself){
  VALUE host, port, timeout, retry, server;
  int ecode;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  host = rb_iv_get(vself, "@host");
  port = rb_iv_get(vself, "@port");
  timeout = rb_iv_get(vself, "@timeout");
  retry = rb_iv_get(vself, "@retry");

  if((!tcrdbtune(db, NUM2DBL(timeout), retry == Qtrue ? RDBTRECON : 0)) ||
     (!tcrdbopen(db, RSTRING_PTR(host), FIX2INT(port)))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoMessengerError, "open error: %s", tcrdberrmsg(ecode));
  }

  server = rb_str_new2(tcrdbexpr(db));
  rb_iv_set(vself, "@server", server);

  return Qtrue;
}

static VALUE mTokyoMessenger_reconnect(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);
  if(db->fd != -1) mTokyoMessenger_close(vself);
  db = tcrdbnew();
  rb_iv_set(vself, RDBVNDATA, Data_Wrap_Struct(rb_cObject, 0, mTokyoMessenger_free, db));

  return mTokyoMessenger_connect(vself);
}

static VALUE mTokyoMessenger_initialize(int argc, VALUE *argv, VALUE vself){
  VALUE host, port, timeout, retry;
  TCRDB *db;

  rb_scan_args(argc, argv, "04", &host, &port, &timeout, &retry);
  if(NIL_P(host)) host = rb_str_new2("127.0.0.1");
  if(NIL_P(port)) port = INT2FIX(1978);
  if(NIL_P(timeout)) timeout = rb_float_new(0.0);
  if(NIL_P(retry)) retry = Qfalse;

  rb_iv_set(vself, "@host", host);
  rb_iv_set(vself, "@port", port);
  rb_iv_set(vself, "@timeout", timeout);
  rb_iv_set(vself, "@retry", retry);

  db = tcrdbnew();
  rb_iv_set(vself, RDBVNDATA, Data_Wrap_Struct(rb_cObject, 0, mTokyoMessenger_free, db));

  return mTokyoMessenger_connect(vself);
}

static VALUE mTokyoMessenger_errmsg(int argc, VALUE *argv, VALUE vself){
  VALUE vecode;
  const char *msg;
  int ecode;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "01", &vecode);

  ecode = (vecode == Qnil) ? tcrdbecode(db) : NUM2INT(vecode);
  msg = tcrdberrmsg(ecode);
  return rb_str_new2(msg);
}

static VALUE mTokyoMessenger_ecode(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return INT2NUM(tcrdbecode(db));
}

static VALUE mTokyoMessenger_out(VALUE vself, VALUE vkey){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  vkey = StringValueEx(vkey);
  return tcrdbout(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)) ? Qtrue : Qfalse;
}

// TODO: merge out and mout?
static VALUE mTokyoMessenger_outlist(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vary, vvalue;
  TCLIST *list, *result;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "*", &vkeys);

  // I really hope there is a better way to do this
  if (RARRAY_LEN(vkeys) == 1) {
    vvalue = rb_ary_entry(vkeys, 0);
    switch (TYPE(vvalue)){
      case T_STRING:
      case T_FIXNUM:
        break;
      case T_ARRAY:
        vkeys = vvalue;
        break;
      case T_OBJECT:
        vkeys = rb_convert_type(vvalue, T_ARRAY, "Array", "to_a");
        break;
    }
  }
  Check_Type(vkeys, T_ARRAY);

  list = varytolist(vkeys);
  result = tcrdbmisc(db, "outlist", 0, list);
  tclistdel(list);
  vary = listtovary(result);
  tclistdel(result);
  return vary;
}

static VALUE mTokyoMessenger_check(VALUE vself, VALUE vkey){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  vkey = StringValueEx(vkey);
  return tcrdbvsiz(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)) >= 0 ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_iterinit(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return tcrdbiterinit(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_iternext(VALUE vself){
  VALUE vval;
  char *vbuf;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  if(!(vbuf = tcrdbiternext2(db))) return Qnil;
  vval = rb_str_new2(vbuf);
  tcfree(vbuf);

  return vval;
}

static VALUE mTokyoMessenger_fwmkeys(int argc, VALUE *argv, VALUE vself){
  VALUE vprefix, vmax, vary;
  TCLIST *keys;
  int max;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "11", &vprefix, &vmax);

  vprefix = StringValueEx(vprefix);
  max = (vmax == Qnil) ? -1 : NUM2INT(vmax);
  keys = tcrdbfwmkeys(db, RSTRING_PTR(vprefix), RSTRING_LEN(vprefix), max);
  vary = listtovary(keys);
  tclistdel(keys);
  return vary;
}

static VALUE mTokyoMessenger_delete_keys_with_prefix(int argc, VALUE *argv, VALUE vself){
  VALUE vprefix, vmax;
  TCLIST *keys;
  int max;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "11", &vprefix, &vmax);

  vprefix = StringValueEx(vprefix);
  max = (vmax == Qnil) ? -1 : NUM2INT(vmax);
  keys = tcrdbfwmkeys(db, RSTRING_PTR(vprefix), RSTRING_LEN(vprefix), max);
  tcrdbmisc(db, "outlist", 0, keys);
  tclistdel(keys);
  return Qnil;
}

static VALUE mTokyoMessenger_keys(VALUE vself){
  /*
  VALUE vary;
  char *kxstr;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    rb_ary_push(vary, rb_str_new2(kxstr));
  }
  return vary;
  */

  // Using forward matching keys with an empty string is 100x faster than iternext+get
  VALUE vary;
  TCLIST *keys;
  char *prefix;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  prefix = "";
  keys = tcrdbfwmkeys2(db, prefix, -1);
  vary = listtovary(keys);
  tclistdel(keys);
  return vary;
}

static VALUE mTokyoMessenger_addint(VALUE vself, VALUE vkey, int inum){
  TCRDB *db = mTokyoMessenger_getdb(vself);
  vkey = StringValueEx(vkey);

  inum = tcrdbaddint(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), inum);
  return inum == INT_MIN ? Qnil : INT2NUM(inum);
}

static VALUE mTokyoMessenger_add_int(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vnum;
  int inum = 1;

  rb_scan_args(argc, argv, "11", &vkey, &vnum);
  vkey = StringValueEx(vkey);
  if(NIL_P(vnum)) vnum = INT2NUM(inum);

  return mTokyoMessenger_addint(vself, vkey, NUM2INT(vnum));
}

static VALUE mTokyoMessenger_get_int(VALUE vself, VALUE vkey){
  return mTokyoMessenger_addint(vself, vkey, 0);
}

static VALUE mTokyoMessenger_adddouble(VALUE vself, VALUE vkey, double dnum){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  vkey = StringValueEx(vkey);
  dnum = tcrdbadddouble(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), dnum);
  return isnan(dnum) ? Qnil : rb_float_new(dnum);
}

static VALUE mTokyoMessenger_add_double(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vnum;
  double dnum = 1.0;

  rb_scan_args(argc, argv, "11", &vkey, &vnum);
  vkey = StringValueEx(vkey);
  if(NIL_P(vnum)) vnum = rb_float_new(dnum);

  return mTokyoMessenger_adddouble(vself, vkey, NUM2DBL(vnum));
}

static VALUE mTokyoMessenger_get_double(VALUE vself, VALUE vkey){
  return mTokyoMessenger_adddouble(vself, vkey, 0.0);
}

static VALUE mTokyoMessenger_sync(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return tcrdbsync(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_optimize(int argc, VALUE *argv, VALUE vself){
  VALUE vparams;
  const char *params = NULL;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "01", &vparams);
  if(NIL_P(vparams)) vparams = Qnil;
  if(vparams != Qnil) params = RSTRING_PTR(vparams);

  return tcrdboptimize(db, params) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_vanish(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return tcrdbvanish(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_copy(VALUE vself, VALUE path){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  Check_Type(path, T_STRING);
  return tcrdbcopy(db, RSTRING_PTR(path)) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_restore(VALUE vself, VALUE vpath, VALUE vts, VALUE vopts){
  uint64_t ts;
  int opts;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  Check_Type(vpath, T_STRING);
  ts = (uint64_t) FIX2INT(vts);
  opts = FIX2INT(vopts);
  return tcrdbrestore(db, RSTRING_PTR(vpath), ts, opts) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_setmst(VALUE vself, VALUE vhost, VALUE vport, VALUE vts, VALUE vopts){
  uint64_t ts;
  int opts;
  TCRDB *db = mTokyoMessenger_getdb(vself);

  ts = (uint64_t) FIX2INT(vts);
  opts = FIX2INT(vopts);
  return tcrdbsetmst(db, RSTRING_PTR(vhost), FIX2INT(vport), ts, opts) ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_rnum(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return LL2NUM(tcrdbrnum(db));
}

static VALUE mTokyoMessenger_empty(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return tcrdbrnum(db) < 1 ? Qtrue : Qfalse;
}

static VALUE mTokyoMessenger_size(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return LL2NUM(tcrdbsize(db));
}

static VALUE mTokyoMessenger_stat(VALUE vself){
  TCRDB *db = mTokyoMessenger_getdb(vself);

  return rb_str_new2(tcrdbstat(db));
}

static VALUE mTokyoMessenger_misc(int argc, VALUE *argv, VALUE vself){
  VALUE vname, vopts, vargs, vary;
  TCLIST *list, *args;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  rb_scan_args(argc, argv, "13", &vname, &vopts, &vargs);

  args = varytolist(vargs);
  vname = StringValueEx(vname);

  list = tcrdbmisc(db, RSTRING_PTR(vname), NUM2INT(vopts), args);
  vary = listtovary(list);
  tclistdel(list);
  return vary;
}

static VALUE mTokyoMessenger_ext(VALUE vself, VALUE vext, VALUE vkey, VALUE vvalue){
  int vsiz;
  char *vbuf;
  TCRDB *db = mTokyoMessenger_getdb(vself);
  vext = StringValueEx(vext);
  vkey = StringValueEx(vkey);
  vvalue = StringValueEx(vvalue);

  if(!(vbuf = tcrdbext(db, RSTRING_PTR(vext), 0, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vvalue), RSTRING_LEN(vvalue), &vsiz))){
    return Qnil;
  } else {
    return rb_str_new(vbuf, vsiz);
  }
}

static VALUE mTokyoMessenger_each_key(VALUE vself){
  VALUE vrv;
  char *kxstr;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  TCRDB *db = mTokyoMessenger_getdb(vself);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    vrv = rb_yield_values(1, rb_str_new2(kxstr));
  }
  return vrv;
}

void init_mod(){
  rb_define_const(mTokyoMessenger, "ESUCCESS", INT2NUM(TTESUCCESS));
  rb_define_const(mTokyoMessenger, "EINVALID", INT2NUM(TTEINVALID));
  rb_define_const(mTokyoMessenger, "ENOHOST", INT2NUM(TTENOHOST));
  rb_define_const(mTokyoMessenger, "EREFUSED", INT2NUM(TTEREFUSED));
  rb_define_const(mTokyoMessenger, "ESEND", INT2NUM(TTESEND));
  rb_define_const(mTokyoMessenger, "ERECV", INT2NUM(TTERECV));
  rb_define_const(mTokyoMessenger, "EKEEP", INT2NUM(TTEKEEP));
  rb_define_const(mTokyoMessenger, "ENOREC", INT2NUM(TTENOREC));
  rb_define_const(mTokyoMessenger, "EMISC", INT2NUM(TTEMISC));

  rb_define_const(mTokyoMessenger, "ITLEXICAL", INT2NUM(RDBITLEXICAL));
  rb_define_const(mTokyoMessenger, "ITDECIMAL", INT2NUM(RDBITDECIMAL));
  rb_define_const(mTokyoMessenger, "ITVOID", INT2NUM(RDBITVOID));
  rb_define_const(mTokyoMessenger, "ITKEEP", INT2NUM(RDBITKEEP));

  rb_define_private_method(mTokyoMessenger, "initialize", mTokyoMessenger_initialize, -1);
  rb_define_private_method(mTokyoMessenger, "connect", mTokyoMessenger_connect, 0);
  rb_define_method(mTokyoMessenger, "reconnect", mTokyoMessenger_reconnect, 0);
  rb_define_method(mTokyoMessenger, "server", mTokyoMessenger_server, 0);
  rb_define_method(mTokyoMessenger, "close", mTokyoMessenger_close, 0);
  rb_define_method(mTokyoMessenger, "errmsg", mTokyoMessenger_errmsg, -1);
  rb_define_method(mTokyoMessenger, "ecode", mTokyoMessenger_ecode, 0);
  rb_define_method(mTokyoMessenger, "out", mTokyoMessenger_out, 1);
  rb_define_alias(mTokyoMessenger, "delete", "out");                 // Rufus Compat
  rb_define_method(mTokyoMessenger, "outlist", mTokyoMessenger_outlist, -1);
  rb_define_alias(mTokyoMessenger, "mdelete", "outlist");
  rb_define_alias(mTokyoMessenger, "ldelete", "outlist");            // Rufus Compat
  rb_define_method(mTokyoMessenger, "check", mTokyoMessenger_check, 1);
  rb_define_alias(mTokyoMessenger, "has_key?", "check");
  rb_define_alias(mTokyoMessenger, "key?", "check");
  rb_define_alias(mTokyoMessenger, "include?", "check");
  rb_define_alias(mTokyoMessenger, "member?", "check");
  rb_define_method(mTokyoMessenger, "iterinit", mTokyoMessenger_iterinit, 0);
  rb_define_method(mTokyoMessenger, "iternext", mTokyoMessenger_iternext, 0);
  rb_define_method(mTokyoMessenger, "fwmkeys", mTokyoMessenger_fwmkeys, -1);
  rb_define_method(mTokyoMessenger, "delete_keys_with_prefix", mTokyoMessenger_delete_keys_with_prefix, -1);// Rufus Compat
  rb_define_alias(mTokyoMessenger, "dfwmkeys", "delete_keys_with_prefix");  
  rb_define_method(mTokyoMessenger, "keys", mTokyoMessenger_keys, 0);
  rb_define_method(mTokyoMessenger, "add_int", mTokyoMessenger_add_int, -1);
  rb_define_alias(mTokyoMessenger, "addint", "add_int");
  rb_define_alias(mTokyoMessenger, "increment", "add_int");
  rb_define_method(mTokyoMessenger, "get_int", mTokyoMessenger_get_int, 1);
  rb_define_method(mTokyoMessenger, "add_double", mTokyoMessenger_add_double, -1);
  rb_define_alias(mTokyoMessenger, "adddouble", "add_double");
  rb_define_method(mTokyoMessenger, "get_double", mTokyoMessenger_get_double, 1);
  rb_define_method(mTokyoMessenger, "sync", mTokyoMessenger_sync, 0);
  rb_define_method(mTokyoMessenger, "optimize", mTokyoMessenger_optimize, -1);
  rb_define_method(mTokyoMessenger, "vanish", mTokyoMessenger_vanish, 0);
  rb_define_alias(mTokyoMessenger, "clear", "vanish");
  rb_define_method(mTokyoMessenger, "copy", mTokyoMessenger_copy, 1);
  rb_define_method(mTokyoMessenger, "restore", mTokyoMessenger_restore, 2);
  rb_define_method(mTokyoMessenger, "setmst", mTokyoMessenger_setmst, 4);
  rb_define_method(mTokyoMessenger, "rnum", mTokyoMessenger_rnum, 0);
  rb_define_alias(mTokyoMessenger, "count", "rnum");
  rb_define_method(mTokyoMessenger, "empty?", mTokyoMessenger_empty, 0);
  rb_define_alias(mTokyoMessenger, "size", "rnum");                    // Rufus Compat
  rb_define_method(mTokyoMessenger, "db_size", mTokyoMessenger_size, 0);  // Rufus Compat
  rb_define_alias(mTokyoMessenger, "length", "size");
  rb_define_method(mTokyoMessenger, "stat", mTokyoMessenger_stat, 0);
  rb_define_method(mTokyoMessenger, "misc", mTokyoMessenger_misc, -1);
  rb_define_method(mTokyoMessenger, "ext", mTokyoMessenger_ext, 3);
  rb_define_alias(mTokyoMessenger, "run", "ext");
  rb_define_method(mTokyoMessenger, "each_key", mTokyoMessenger_each_key, 0);
}