// #define _GNU_SOURCE

#include <arpa/inet.h>
#include <assert.h>
#include <gmodule.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

#include "./flag.h"

// https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define xstr(a) str(a)
#define str(a) #a

// Unicode character might take up to 4 bytes each
#define SZ        1024
#define SZ_CIPHER  64
#define SZ_PORT     8
// #define FILENAME "/home/darren/yaml/01_rixcloud.yaml"
// #define FILENAME "/home/darren/yaml/01_ssrcloud.yaml"

#define eprintf(...) fprintf(stderr,__VA_ARGS__)
#define LAMBDA(X) ({ X f;})

#define SCAN() assert(1==yaml_parser_scan(&parser,&token))
#define DEL() yaml_token_delete(&token);token=(yaml_token_t){}

#define EMIT()      assert(1==yaml_emitter_emit(&emitter, &event))
#define SCALAR(S)   assert(S);yaml_scalar_event_initialize(&event,NULL,(yaml_char_t*)YAML_STR_TAG,(const yaml_char_t*)S,strlen(S),1,1,YAML_PLAIN_SCALAR_STYLE /*YAML_SINGLE_QUOTED_SCALAR_STYLE*/);EMIT()
#define MAP_END()   yaml_mapping_end_event_initialize(&event);EMIT()
#define SEQ_END()   yaml_sequence_end_event_initialize(&event);EMIT()
#define MAP_START() yaml_mapping_start_event_initialize(&event,NULL,(yaml_char_t *)YAML_MAP_TAG,1,YAML_BLOCK_MAPPING_STYLE /*YAML_ANY_MAPPING_STYLE*/);EMIT()
#define SEQ_START() yaml_sequence_start_event_initialize(&event,NULL,(yaml_char_t *)YAML_SEQ_TAG,1,YAML_BLOCK_SEQUENCE_STYLE /*YAML_ANY_SEQUENCE_STYLE*/);EMIT()

#ifdef FILENAME
static FILE *file=NULL;
#endif

static yaml_parser_t parser={};
static yaml_token_t token={};

static yaml_emitter_t emitter={};
static yaml_event_t event={};

static GSList *l_asia=NULL;
static GSList *l_inv_hk_ru=NULL;
static GSList *l_jp=NULL;
static GSList *l_ca_us=NULL;
static GSList *l_eu_gb=NULL;
static GSList *l_xx=NULL;

// name:     // var
// type: ss  // const
// server:   // var
// port:     // shared // static char plain_port[SZ_PORT]
// cipher:   // shared // static char plain_cipher[SZ_CIPHER]
// password: // shared // static char plain_password[SZ]
// udp: true // const

// Common
static char plain_port[SZ_PORT]={};
static char plain_cipher[SZ_CIPHER]={};
static char plain_password[SZ]={};

// name:          // var
// type: ss       // const
// server:        // var
// port:          // var
// cipher:        // shared // static char obfs_cipher[SZ_CIPHER]
// password: CNIX // const
// udp: true      // const
// plugin: obfs   // const
// plugin-opts:
//   mode: http   // const
//   host:        // shared // static char obfs_host[SZ]

// Unique
typedef struct _Node {
  bool obfs;
  char name[SZ];
  char server[SZ];
} Node;

static bool strstrVA(const char *const haystack, ...){
  va_list ap;
  va_start(ap,haystack);
  const char *needle=NULL;
  while((needle=(const char*)va_arg(ap,const char*)))
    if(strstr(haystack,needle))
      return true;
  return false;
}

static void group(const char *const s){

  #define G_SLIST_PREPEND(l,e) l=g_slist_prepend(l,e)
  bool xx=true;

  //                                        hk   jp    kr                ru              sg      tw
  if(strstrVA(s,"ASYNCHRONOUS TRANSFERMODE","港","日本","韓國","韓国","韩国","俄羅斯","俄罗斯","新加坡","臺灣","台灣","台湾",NULL))
                                                                     { G_SLIST_PREPEND(l_asia      ,strdup(s)); xx=false; }

  if(!strstrVA(s,"ASYNCHRONOUS TRANSFERMODE","港","俄羅斯","俄罗斯",NULL))
                                                                     { G_SLIST_PREPEND(l_inv_hk_ru ,strdup(s));              }
  if(strstr(s,"日本")||strcasestr(s,"ntt"))/*補補補*/                  { G_SLIST_PREPEND(l_jp        ,strdup(s)); xx=false; }
  if(strstrVA(s,"美國","美国","加拿大",NULL))/*補補補*/                  { G_SLIST_PREPEND(l_ca_us     ,strdup(s)); xx=false; }
  if(strstrVA(s,"荷蘭","荷兰","德國","德国","英國","英国",NULL))/*補補補*/ { G_SLIST_PREPEND(l_eu_gb     ,strdup(s)); xx=false; }
  if(xx)/**/                                                      { G_SLIST_PREPEND(l_xx     ,strdup(s));              }

  #undef G_SLIST_PREPEND

}

static void emit_scalar(const char *s,...){
  va_list vl;
  va_start(vl,s);
  while(s!=NULL){
    SCALAR(s);
    s=va_arg(vl,const char *);
  }
}

static void emitter_begin(){
  // Init
  yaml_emitter_initialize(&emitter);
  yaml_emitter_set_output_file(&emitter,stdout);
  yaml_emitter_set_encoding(&emitter,YAML_UTF8_ENCODING);
  yaml_emitter_set_canonical(&emitter,0);
  yaml_emitter_set_indent(&emitter,2);
  yaml_emitter_set_width(&emitter,-1);
  yaml_emitter_set_unicode(&emitter,1);
  yaml_emitter_set_break(&emitter,YAML_LN_BREAK);
  // Header
  yaml_stream_start_event_initialize(&event,YAML_UTF8_ENCODING);
  EMIT();
  yaml_document_start_event_initialize(&event,/*&((yaml_version_directive_t){1,2})*/NULL,NULL,NULL,1);
  EMIT();
  MAP_START();
  emit_scalar(
    "port", "8080",
    "socks-port", "1080",
    "allow-lan", "true",
    "bind-address", "*",
    "mode", "global",
    "log-level", "info",
    "ipv6", "false",
    "external-controller", "127.0.0.1:"xstr(RESTFUL_PORT), // yacd updated defaults
    // https://github.com/haishanh/yacd/issues/612
    // Access at "http://127.0.0.1:RESTFUL_PORT/ui/index.html"
    // Groups are at the bottom of GLOBAL
    "external-ui", "/tmp/yacd-gh-pages",
    "secret", "",
    // "interface-name", get_gateway_iface(),
    NULL
  );
  SCALAR("profile");MAP_START();
    SCALAR("store-selected");SCALAR("false");
  MAP_END();
  SCALAR("dns");MAP_START();
    SCALAR("enable");SCALAR("false");
  MAP_END();
}

static void emitter_end(){
  MAP_END();
  yaml_document_end_event_initialize(&event,1);
  EMIT();
  yaml_stream_end_event_initialize(&event);
  EMIT();
  yaml_emitter_delete(&emitter);
  emitter=(yaml_emitter_t){};
}

/*static void type(const yaml_token_type_t t){
  switch(t){
    case YAML_NO_TOKEN:eprintf("YAML_NO_TOKEN\n");break;
    case YAML_STREAM_START_TOKEN:eprintf("YAML_STREAM_START_TOKEN\n");break;
    case YAML_STREAM_END_TOKEN:eprintf("YAML_STREAM_END_TOKEN\n");break;
    case YAML_VERSION_DIRECTIVE_TOKEN:eprintf("YAML_VERSION_DIRECTIVE_TOKEN\n");break;
    case YAML_TAG_DIRECTIVE_TOKEN:eprintf("YAML_TAG_DIRECTIVE_TOKEN\n");break;
    case YAML_DOCUMENT_START_TOKEN:eprintf("YAML_DOCUMENT_START_TOKEN\n");break;
    case YAML_DOCUMENT_END_TOKEN:eprintf("YAML_DOCUMENT_END_TOKEN\n");break;
    case YAML_BLOCK_SEQUENCE_START_TOKEN:eprintf("YAML_BLOCK_SEQUENCE_START_TOKEN\n");break;
    case YAML_BLOCK_MAPPING_START_TOKEN:eprintf("YAML_BLOCK_MAPPING_START_TOKEN\n");break;
    case YAML_BLOCK_END_TOKEN:eprintf("YAML_BLOCK_END_TOKEN\n");break;
    case YAML_FLOW_SEQUENCE_START_TOKEN:eprintf("YAML_FLOW_SEQUENCE_START_TOKEN\n");break;
    case YAML_FLOW_SEQUENCE_END_TOKEN:eprintf("YAML_FLOW_SEQUENCE_END_TOKEN\n");break;
    case YAML_FLOW_MAPPING_START_TOKEN:eprintf("YAML_FLOW_MAPPING_START_TOKEN\n");break;
    case YAML_FLOW_MAPPING_END_TOKEN:eprintf("YAML_FLOW_MAPPING_END_TOKEN\n");break;
    case YAML_BLOCK_ENTRY_TOKEN:eprintf("YAML_BLOCK_ENTRY_TOKEN\n");break;
    case YAML_FLOW_ENTRY_TOKEN:eprintf("YAML_FLOW_ENTRY_TOKEN\n");break;
    case YAML_KEY_TOKEN:eprintf("YAML_KEY_TOKEN\n");break;
    case YAML_VALUE_TOKEN:eprintf("YAML_VALUE_TOKEN\n");break;
    case YAML_ALIAS_TOKEN:eprintf("YAML_ALIAS_TOKEN\n");break;
    case YAML_ANCHOR_TOKEN:eprintf("YAML_ANCHOR_TOKEN\n");break;
    case YAML_TAG_TOKEN:eprintf("YAML_TAG_TOKEN\n");break;
    case YAML_SCALAR_TOKEN:eprintf("YAML_SCALAR_TOKEN\n");break;
    default:eprintf("UNKNOWN\n");break;
  }
}*/

static void assert_token_type(yaml_token_type_t tt){
  SCAN();
  assert(token.type==tt);
  DEL();
}

static void parser_begin(){

  yaml_parser_initialize(&parser);
  yaml_parser_set_encoding(&parser,YAML_UTF8_ENCODING);

  #ifdef FILENAME
  file=fopen(FILENAME,"r");
  yaml_parser_set_input_file(&parser,file);
  #else
  yaml_parser_set_input_file(&parser,stdin);
  #endif

  assert_token_type(YAML_STREAM_START_TOKEN);
  assert_token_type(YAML_BLOCK_MAPPING_START_TOKEN);

  // Skip raw header
  for(;;){
    SCAN();
    if(token.type==YAML_KEY_TOKEN){
      DEL();
      SCAN();
      if(token.type==YAML_SCALAR_TOKEN)
        if(0==strcmp((const char*)(token.data.scalar.value),"proxies")){
          DEL();
          break;
        }
    }
    DEL();
  }

  assert_token_type(YAML_VALUE_TOKEN);
  assert_token_type(YAML_BLOCK_SEQUENCE_START_TOKEN);

}

static void parser_end(){

  yaml_parser_delete(&parser);
  parser=(yaml_parser_t){};

  #ifdef FILENAME
  fclose(file);
  file=NULL;
  #endif

}

static void extract_value_with_key(const char *const k,char *v){
  assert_token_type(YAML_KEY_TOKEN);
  SCAN();
  assert(token.type==YAML_SCALAR_TOKEN);
  assert(0==strcmp((const char*)(token.data.scalar.value),k));
  DEL();
  assert_token_type(YAML_VALUE_TOKEN);
  SCAN();
  assert(token.type==YAML_SCALAR_TOKEN);
  strcpy(v,(const char*)(token.data.scalar.value));
  DEL();
}

static void assert_value_with_key(const char *const k,const char *const v0){
  assert_token_type(YAML_KEY_TOKEN);
  SCAN();
  assert(token.type==YAML_SCALAR_TOKEN);
  assert(0==strcmp((const char*)(token.data.scalar.value),k));
  DEL();
  assert_token_type(YAML_VALUE_TOKEN);
  SCAN();
  assert(token.type==YAML_SCALAR_TOKEN);
  assert(0==strcmp((const char*)(token.data.scalar.value),v0));
  DEL();
}

/*static void skip_node(){
  bool test=false;
  for(;;){
    SCAN();
    type(token.type);
    if(token.type==YAML_BLOCK_MAPPING_START_TOKEN){
      for(int i=0;i<20;++i){
        DEL();
        SCAN();
        type(token.type);
      }
      assert(false);
    }
    if(token.type==YAML_BLOCK_END_TOKEN){
      DEL();
      break;
    }
    DEL();
  }
}*/


// lean: Expected n(YAML_BLOCK_END_TOKEN) - n(YAML_BLOCK_MAPPING_START_TOKEN)
static void skip_node(int lean){
  for(;;){
    SCAN();
    if(token.type==YAML_BLOCK_MAPPING_START_TOKEN)
      ++lean;
    if(token.type==YAML_BLOCK_END_TOKEN){
      if(lean==0){
        DEL();
        break;
      }
      assert(lean>=1);
      --lean;
    }
    DEL();
  }
}

static void emit_node(Node *n){
  assert(!(n->obfs));
  MAP_START();
  emit_scalar(
    "name",n->name,
    "type","ss",
    "server",n->server,
    "port",plain_port,
    "cipher",plain_cipher,
    "password",plain_password,
    "udp","true",
    NULL
  );
  MAP_END();
}

/*static void lambda1(gpointer data, __attribute__((__unused__)) gpointer user_data){
  SCALAR(data);
}*/

static void emit_and_destroy_group(const char *const title,GSList **const l){

  if(!(*l)){
    eprintf("skip empty group %s\n",title);
    return;
  }

  MAP_START();

  emit_scalar(
    "name",title,
    "type","select",
    NULL
  );

  SCALAR("proxies");

  SEQ_START();

  // g_slist_foreach(*l,&lambda1,NULL);
  g_slist_foreach(
    *l,
    LAMBDA(void f(gpointer data, __attribute__((__unused__)) gpointer user_data){
      SCALAR(data);
    }),
    NULL
  );

  g_slist_free_full(*l,g_free);
  *l=NULL;

  SEQ_END();

  MAP_END();

}

/*static void test(){

  // FlagMBS m={};
  // cc2wcs2mbs(&m,&(CC){"US"});

  // assert(0==fwide(stdout,0));
  // printf("%s\n",m.mbs);

  // const wchar_t *name = L"r\u00e9sum\u00e9";
  // const wchar_t *name = L"\U0001F1ED\U0001F1F0";
  // wprintf(L"name is %ls\n",name);

  // https://stackoverflow.com/questions/8681623/printf-and-wprintf-in-single-c-code
  // fwide(3)
}*/

int main(const int argc, const char **argv){

  assert(argv);

  if(argc!=1){
    puts("");
    printf("  %s <in.yaml [>out.yaml]\n",argv[0]);
    puts("");
    return 0;
  }

  setlocale(LC_CTYPE,"en_US.UTF-8");

  // test();
  // eprintf("\n");
  // exit(0);

  emitter_begin();
  parser_begin();

  SCALAR("proxies");

  SEQ_START();
  // eprintf("A\n");

  for(;;){

    // eprintf("B\n");

    // If sequence has ended
    SCAN();
    if(token.type==YAML_BLOCK_END_TOKEN)
      break;
    if(token.type!=YAML_BLOCK_ENTRY_TOKEN)
      assert(false);

    assert_token_type(YAML_BLOCK_MAPPING_START_TOKEN);

    Node node={};

    extract_value_with_key("name",node.name);
    if(
      strstr(node.name,"回國")||strstr(node.name,"中國標準 BGP 邊緣")||
      strstr(node.name,"回国")||strstr(node.name,"中国标准 BGP 边缘")
    ){
      eprintf("drop cn node %s\n",node.name);
      skip_node(0);
      continue;
    }
    char type[8]={};
    extract_value_with_key("type",type);
    if(0!=strcmp("ss",type)){
      eprintf("drop %s node %s\n",type,node.name);
      skip_node(0);
      continue;
    }
    extract_value_with_key("server",node.server);
    if(1==inet_pton(AF_INET6,node.server,&((struct in6_addr){}))){
      eprintf("drop IPv6 node %s %s\n",node.name,node.server);
      skip_node(0);
      continue;
    }
    if(0==strcmp("127.0.0.1",node.server)){
      eprintf("drop loopback node %s\n",node.name);
      skip_node(0);
      continue;
    }

    char port[SZ_PORT]={};
    char cipher[SZ_CIPHER]={};
    char password[SZ]={};
    extract_value_with_key("port",port);
    extract_value_with_key("cipher",cipher);
    extract_value_with_key("password",password);

    assert_value_with_key("udp","true");

    SCAN();

    // Plain
    if(token.type==YAML_BLOCK_END_TOKEN){

      node.obfs=false;
      if(0==strlen(plain_port)&&0==strlen(plain_cipher)&&0==strlen(plain_password)){
        strcpy(plain_port,port);
        strcpy(plain_cipher,cipher);
        strcpy(plain_password,password);
        eprintf("\n");
        eprintf("plain port     - %s\n",plain_port);
        eprintf("plain cipher   - %s\n",plain_cipher);
        eprintf("plain password - %s\n",plain_password);
        eprintf("\n");
      }else if(3<=strlen(plain_port)&&3<=strlen(plain_cipher)&&3<=strlen(plain_password)){
        assert(0==strcmp(port,plain_port));
        assert(0==strcmp(cipher,plain_cipher));
        assert(0==strcmp(password,plain_password));
      }else{
        assert(false);
      }

    // Obfs
    }else if(token.type==YAML_KEY_TOKEN){

      eprintf("drop obfs node %s %s\n",node.name,node.server);
      skip_node(0);
      continue;

      // node.obfs=true;

      // assert(0==strcmp(password,"CNIX"));

      // eprintf("\n");
      // eprintf("obfs cipher - %s\n",obfs_cipher);
      // eprintf("obfs port   - %s\n",obfs_port);
      // eprintf("obfs host   - %s\n",obfs_host);
      // eprintf("\n");

    // Error
    }else{

      assert(false);

    }

    DEL();

    group(node.name);
    emit_node(&node);

  }

  SEQ_END();

  SCALAR("proxy-groups");

  SEQ_START();

  assert(MB_CUR_MAX==6);

  char buf[BUF_SZ];
  bzero(buf,BUF_SZ);

  // ccs2str(buf,
  //   &(CC){"UN"},
  // NULL);emit_and_destroy_group(buf,&l_xx);
  /***/;emit_and_destroy_group("XX",&l_xx);

  /***/;emit_and_destroy_group("INV_HK_RU",&l_inv_hk_ru);

  ccs2str(buf,
    &(CC){"EU"},
    &(CC){"GB"},
  NULL);emit_and_destroy_group(buf,&l_eu_gb);

  ccs2str(buf,
    &(CC){"JP"},
  NULL);emit_and_destroy_group(buf,&l_jp);

  ccs2str(buf,
    &(CC){"CA"},
    &(CC){"US"},
  NULL);emit_and_destroy_group(buf,&l_ca_us);

  ccs2str(buf,
    &(CC){"HK"},
    &(CC){"JP"},
    &(CC){"KR"},
    &(CC){"RU"},
    &(CC){"SG"},
    &(CC){"TW"},
  NULL);emit_and_destroy_group(buf,&l_asia);

  SEQ_END();

  parser_end();

  emitter_end();

}
