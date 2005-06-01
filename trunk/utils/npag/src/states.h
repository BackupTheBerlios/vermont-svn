

void begin			__P((char *word, struct s_config *conf, struct s_automata *automata));


void read_stream	__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_stream	__P((struct s_config*));
void check_stream	__P((struct s_config*));

void read_ip		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_ip		__P((struct s_config*));
void check_ip		__P((struct s_config*));

void read_ip6		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_ip6		__P((struct s_config*));
void check_ip6		__P((struct s_config*));

void read_tcp		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_tcp		__P((struct s_config*));
void check_tcp		__P((struct s_config*));

void read_udp		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_udp		__P((struct s_config*));
void check_udp		__P((struct s_config*));

void read_icmp		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_icmp		__P((struct s_config*));
void check_icmp		__P((struct s_config*));

void read_icmp6		__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_icmp6		__P((struct s_config*));
void check_icmp6	__P((struct s_config*));

void read_traffic	__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_traffic	__P(());
void check_traffic	__P((struct s_config*));

void read_pattern	__P((char *word, char *val, struct s_config *conf, struct s_automata *automata));
void init_pattern	__P((struct s_config*));
void check_pattern	__P((struct s_config*));

