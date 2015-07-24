struct mod {
  int id;
  char *buf;
  off_t size;
  char *name, *fullname;
  struct mod *next;
};

struct prg {
  char *name;
  int fd;
  char *buf;
  off_t size;
  int status;
  int argc;
  char **argv;
  struct mod *main;
  struct mod *modules;
};

int prg_register(struct prg *prg);
int prg_wrapped_compile_execute(duk_context *ctx);
int prg_push_modsearch(duk_context *ctx);
int prg_parse_appfile(struct prg *prg);
struct mod *prg_storage_byname(const char *name);
struct mod *prg_storage_byid(int id);
int prg1(duk_context *ctx);
