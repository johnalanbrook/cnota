#include "quickjs.h"

#define NOTA_IMPLEMENTATION
#include "nota.h"

char *js_do_nota_decode(JSContext *js, JSValue *tmp, char *nota)
{
  int type = nota_type(nota);
  JSValue ret2;
  long long n;
  double d;
  int b;
  char *str;

  switch(type) {
    case NOTA_BLOB:
      break;
    case NOTA_TEXT:
      nota = nota_read_text(&str, nota);
      *tmp = JS_NewString(js, str);
      /* TODO: Avoid malloc and free here */
      free(str);
      break;
    case NOTA_ARR:
      nota = nota_read_array(&n, nota);
      *tmp = JS_NewArray(js);
      for (int i = 0; i < n; i++) {
        nota = js_do_nota_decode(js, &ret2, nota);
        JS_SetPropertyInt64(js, *tmp, i, ret2);
      }
      break;
    case NOTA_REC:
      nota = nota_read_record(&n, nota);
      *tmp = JS_NewObject(js);
      for (int i = 0; i < n; i++) {
        nota = nota_read_text(&str, nota);
        nota = js_do_nota_decode(js, &ret2, nota);
        JS_SetPropertyStr(js, *tmp, str, ret2);
        free(str);
      }
      break;
    case NOTA_INT:
      nota = nota_read_int(&n, nota);
      *tmp = JS_NewInt64(js,n);
      break;
    case NOTA_SYM:
      nota = nota_read_sym(&b, nota);
      if (b == NOTA_NULL) *tmp = JS_UNDEFINED;
      else
        *tmp = JS_NewBool(js,b);
      break;
    default:
    case NOTA_FLOAT:
      nota = nota_read_float(&d, nota);
      *tmp = JS_NewFloat64(js,d);
      break;
  }

  return nota;
}

// Writers the JSValue v into the buffer of char *nota, returning a pointer to the next byte in nota to be written
char *js_do_nota_encode(JSContext *js, JSValue v, char *nota)
{
  int tag = JS_VALUE_GET_TAG(v);
  const char *str = NULL;
  JSPropertyEnum *ptab;
  uint32_t plen;
  int n;
  double nval;
  JSValue val;

  switch(tag) {
    case JS_TAG_FLOAT64:
    case JS_TAG_INT:
      JS_ToFloat64(js, &nval, v);
      return nota_write_number(nval, nota);
    case JS_TAG_STRING:
      str = JS_ToCString(js, v);
      nota = nota_write_text(str, nota);
      JS_FreeCString(js, str);
      return nota;
    case JS_TAG_BOOL:
      return nota_write_sym(JS_VALUE_GET_BOOL(v), nota);
    case JS_TAG_UNDEFINED:
      return nota_write_sym(NOTA_NULL, nota);
    case JS_TAG_NULL:
      return nota_write_sym(NOTA_NULL, nota);
    case JS_TAG_OBJECT:
      if (JS_IsArray(js, v)) {
        int n;
        JS_ToInt32(js, &n, JS_GetPropertyStr(js, v, "length"));
        nota = nota_write_array(n, nota);
        for (int i = 0; i < n; i++)
          nota = js_do_nota_encode(js, JS_GetPropertyUint32(js, v, i), nota);
        return nota;
      }
      n = JS_GetOwnPropertyNames(js, &ptab, &plen, v, JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK);
      nota = nota_write_record(plen, nota);

      for (int i = 0; i < plen; i++) {
        val = JS_GetProperty(js,v,ptab[i].atom);
        str = JS_AtomToCString(js, ptab[i].atom);
        JS_FreeAtom(js, ptab[i].atom);

        nota = nota_write_text(str, nota);
        JS_FreeCString(js, str);

        nota = js_do_nota_encode(js, val, nota);
        JS_FreeValue(js,val);
      }
      js_free(js, ptab);
      return nota;
    default:
      return nota;
  }
  return nota;
}

JSValue js_nota_encode(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  if (argc < 1)
    JS_ThrowInternalError(js, "Expected at least one argument to encode.");

  JSValue obj = argv[0];
  char nota[1024*1024];
  char *e = js_do_nota_encode(js, obj, nota);
  return JS_NewArrayBufferCopy(js, (unsigned char*)nota, e-nota);
}

JSValue js_nota_decode(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  if (argc < 1) return JS_UNDEFINED;

  size_t len;
  unsigned char *nota = JS_GetArrayBuffer(js, &len, argv[0]);
  JSValue ret;
  js_do_nota_decode(js, &ret, (char*)nota);
  return ret;
}

JSValue js_nota_hex(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  size_t len;
  unsigned char *nota = JS_GetArrayBuffer(js, &len, argv[0]);
  print_nota_hex(nota);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_nota_funcs[] = {
  JS_CFUNC_DEF("encode", 1, js_nota_encode),
  JS_CFUNC_DEF("decode", 1, js_nota_decode),
  JS_CFUNC_DEF("hex", 1, js_nota_hex),
};

static int js_nota_init(JSContext *ctx, JSModuleDef *m) {
    JS_SetModuleExportList(ctx, m, js_nota_funcs, sizeof(js_nota_funcs)/sizeof(JSCFunctionListEntry));
    return 0;
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_nota
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
    JSModuleDef *m = JS_NewCModule(ctx, module_name, js_nota_init);
    if (!m) return NULL;
    JS_AddModuleExportList(ctx, m, js_nota_funcs, sizeof(js_nota_funcs)/sizeof(JSCFunctionListEntry));
    return m;
}
