#ifndef CONVERTER_H
#define CONVERTER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>

#ifdef _WIN32
  #include <windows.h>
  #define PLATFORM_WINDOWS
#else
  #include <unistd.h>
  #include <sys/resource.h>
  #define PLATFORM_UNIX
#endif

#define R               "\x1b[0m"
#define BOLD            "\x1b[1m"
#define DIM             "\x1b[2m"
#define BLINK           "\x1b[5m"
#define UNDERLINE       "\x1b[4m"
#define REVERSE         "\x1b[7m"

#define CG1             "\x1b[38;5;46m"    /* Bright matrix green       */
#define CG2             "\x1b[38;5;82m"    /* Neon lime                 */
#define CG3             "\x1b[38;5;40m"    /* Deep matrix green         */
#define CG4             "\x1b[38;5;22m"    /* Dark forest               */

#define CC1             "\x1b[38;5;51m"    /* Neon cyan                 */
#define CC2             "\x1b[38;5;87m"    /* Ice cyan                  */
#define CC3             "\x1b[38;5;27m"    /* Electric blue             */
#define CC4             "\x1b[38;5;33m"    /* Sky blue                  */

#define CM1             "\x1b[38;5;201m"   /* Hot magenta               */
#define CM2             "\x1b[38;5;129m"   /* Purple                    */
#define CM3             "\x1b[38;5;213m"   /* Neon pink                 */
#define CM4             "\x1b[38;5;105m"   /* Violet                    */

#define CY1             "\x1b[38;5;226m"   /* Neon yellow               */
#define CO1             "\x1b[38;5;208m"   /* Orange                    */
#define CR1             "\x1b[38;5;196m"   /* Error red                 */
#define CR2             "\x1b[38;5;160m"   /* Dark red                  */
#define CGO             "\x1b[38;5;220m"   /* Gold                      */
#define CS1             "\x1b[38;5;255m"   /* Near-white                */
#define CGR             "\x1b[38;5;242m"   /* Mid gray                  */
#define CGD             "\x1b[38;5;235m"   /* Dark gray (subtle)        */

#define BG_BLK          "\x1b[40m"
#define BG_D232         "\x1b[48;5;232m"
#define BG_D234         "\x1b[48;5;234m"
#define BG_D236         "\x1b[48;5;236m"

#define CURSOR_UP(n)    "\x1b[" #n "A"
#define CURSOR_DOWN(n)  "\x1b[" #n "B"
#define CURSOR_RIGHT(n) "\x1b[" #n "C"
#define CURSOR_LEFT(n)  "\x1b[" #n "D"
#define CURSOR_HIDE     "\x1b[?25l"
#define CURSOR_SHOW     "\x1b[?25h"
#define CLEAR_LINE      "\x1b[2K\r"
#define CLEAR_SCREEN    "\x1b[2J\x1b[H"
#define APP_VERSION         "2.0.0"
#define APP_DEVELOPER       "Devansh"
#define APP_GITHUB          "https://github.com/codewithdev9/"
#define APP_DONATE          "https://buymeacoffee.com/devansh"
#define APP_NAME            "GLOBAL CURRENCY & FINANCIAL COMMAND CENTER"
#define APP_TAGLINE         "Precision. Speed. Cyberpunk."
#define APP_BUILD_DATE      __DATE__ " " __TIME__

#define LEN_CODE            8
#define LEN_NAME            72
#define LEN_SYMBOL          8
#define LEN_COUNTRY         72
#define LEN_INPUT           512
#define LEN_PIN_HASH        64
#define LEN_API_KEY         256
#define LEN_FMTBUF          128
#define LEN_LOG_MSG         1024

#define INIT_DB_CAPACITY    64
#define INIT_HIST_CAPACITY  64
#define MAX_FAVORITES       20
#define MAX_WALLET_SLOTS    64
#define MAX_ALERTS          32
#define MAX_CMD_TOKENS      16
#define MAX_BULK_TARGETS    32
#define MAX_TRIVIA          25
#define MAX_QUOTES          25
#define SMA_WINDOW_MAX      30

#define CACHE_TTL_SEC       86400   /* 24 h cache                */
#define AUTO_REFRESH_SEC    300     /* 5-minute auto-refresh     */
#define INACTIVITY_SEC      600     /* 10-minute auto-shutdown   */
#define ANIM_FRAME_MS       80      /* Animation frame delay     */

#define FILE_LOG            "converter.log"
#define FILE_CACHE          "rates_cache.bin"
#define FILE_CONFIG         "converter.ini"
#define FILE_HISTORY_CSV    "conversion_history.csv"
#define FILE_BACKUP         "db_backup.bin"
#define FILE_WALLET         "wallet.bin"

#define MASTER_BADGE_AT     100

/* UI layout */
#define UI_WIDTH            78
#define BAR_WIDTH           40
#define GRAPH_HEIGHT        8

typedef enum RoundingMode {
    ROUND_NONE      = 0,
    ROUND_FLOOR     = 1,
    ROUND_CEILING   = 2,
    ROUND_MIDPOINT  = 3,
    ROUND_BANKER    = 4     /* Banker's / Round-half-to-even */
} RoundingMode;


typedef enum {
    APP_LANG_ENGLISH = 0,
    APP_LANG_HINDI,        
    APP_LANG_SPANISH       
} Language;

typedef enum ColorTheme {
    THEME_CYBERPUNK = 0,
    THEME_MATRIX    = 1,
    THEME_NEON      = 2
} ColorTheme;

typedef enum CurrencyType {
    CTYPE_FIAT      = 0,
    CTYPE_CRYPTO    = 1,
    CTYPE_COMMODITY = 2,
    CTYPE_SYNTHETIC = 3
} CurrencyType;

typedef enum LogLevel {
    LOG_INFO    = 0,
    LOG_SUCCESS = 1,
    LOG_WARNING = 2,
    LOG_ERROR   = 3
} LogLevel;

typedef enum SortField {
    SORT_BY_CODE  = 0,
    SORT_BY_NAME  = 1,
    SORT_BY_RATE  = 2,
    SORT_BY_CHANGE= 3
} SortField;

typedef enum SortOrder {
    SORT_ASC  = 0,
    SORT_DESC = 1
} SortOrder;

typedef struct Currency {
    char         code[LEN_CODE];        /* ISO 4217, e.g. "USD"          */
    char         name[LEN_NAME];        /* Full name                     */
    char         symbol[LEN_SYMBOL];    /* e.g. "$", "₹", "€"           */
    char         country[LEN_COUNTRY];  /* Home country                  */
    double       rate_usd;              /* Units per 1 USD               */
    double       change_24h;            /* 24-h % change                 */
    double       change_1h;             /* 1-h % change                  */
    double       high_24h;              /* 24-h high in USD              */
    double       low_24h;               /* 24-h low in USD               */
    double       volume_24h;            /* Trading volume                */
    double       market_cap;            /* Crypto: market cap USD        */
    double       circulating_supply;    /* Crypto: supply                */
    CurrencyType type;
    bool         is_favorite;
    time_t       last_updated;
} Currency;
typedef struct ConversionRecord {
    char         from[LEN_CODE];
    char         to[LEN_CODE];
    double       amount;
    double       result;
    double       rate_used;
    double       markup_pct;
    double       tax_pct;
    RoundingMode rounding;
    time_t       ts;
} ConversionRecord;

/* ─── 4c. WalletEntry ─── */
typedef struct WalletEntry {
    char   code[LEN_CODE];
    double balance;
    double cost_basis_usd;   /* Average purchase price basis */
} WalletEntry;

/* ─── 4d. PriceAlert ─── */
typedef struct PriceAlert {
    char   code[LEN_CODE];
    double target_rate;
    bool   alert_above;     /* true → fire when rate ≥ target */
    bool   triggered;
    time_t set_at;
} PriceAlert;

/* ─── 4e. TechnicalData ─── */
typedef struct TechnicalData {
    double sma_7;
    double sma_14;
    double sma_30;
    double pivot;
    double support1;
    double support2;
    double resistance1;
    double resistance2;
    double volatility_1h;
    double avg_weekly;
} TechnicalData;

/* ─── 4f. ManualRate (user override) ─── */
typedef struct ManualRate {
    char   from[LEN_CODE];
    char   to[LEN_CODE];
    double rate;
    bool   active;
} ManualRate;

/* ─── 4g. GoalTracker ─── */
typedef struct GoalTracker {
    char   from[LEN_CODE];
    char   to[LEN_CODE];
    double target_foreign;
    double saved_local;
    bool   active;
} GoalTracker;

typedef struct FinancialEngine {

    /* ── Currency Database (heap-allocated, growable) ── */
    Currency    *db;
    int          db_count;
    int          db_capacity;

    /* ── Conversion History (heap-allocated, growable) ── */
    ConversionRecord *history;
    int          hist_count;
    int          hist_capacity;
    int          total_conversions;   /* Lifetime counter → badge check */

    /* ── Wallet ── */
    WalletEntry  wallet[MAX_WALLET_SLOTS];
    int          wallet_count;

    /* ── Favorites ── */
    char         favorites[MAX_FAVORITES][LEN_CODE];
    int          fav_count;

    /* ── Price Alerts ── */
    PriceAlert   alerts[MAX_ALERTS];
    int          alert_count;

    /* ── Manual Rate Override ── */
    ManualRate   manual;

    /* ── Goal Tracker ── */
    GoalTracker  goal;

    /* ── Precision & Rounding ── */
    int          precision;         /* 2 or 4 decimal places           */
    RoundingMode rounding;

    /* ── Fees ── */
    double       default_markup;    /* e.g. 1.5 for 1.5%              */
    double       default_tax;       /* e.g. 18.0 for 18% GST          */

    /* ── UI / UX Settings ── */
    Language     language;
    ColorTheme   theme;
    bool         dark_mode;
    bool         show_symbols;
    bool         sound_enabled;

    /* ── Modes ── */
    bool         offline_mode;
    bool         ghost_mode;        /* No log writes                   */
    bool         hacker_mode;       /* Easter-egg activated            */
    bool         auto_update;
    bool         is_locked;

    /* ── Session ── */
    char         base_currency[LEN_CODE];
    char         last_from[LEN_CODE];
    char         last_to[LEN_CODE];
    double       last_amount;
    time_t       last_activity;
    time_t       session_start;

    /* ── Cache ── */
    time_t       cache_ts;
    bool         cache_valid;

    /* ── Security ── */
    char         pin_hash[LEN_PIN_HASH];
    char         api_key_masked[LEN_API_KEY];
    char         api_key[LEN_API_KEY];

} FinancialEngine;

FinancialEngine *engine_init(void);
void             engine_destroy(FinancialEngine *e);
bool             engine_add_currency(FinancialEngine *e, const Currency *c);
bool             engine_grow_db(FinancialEngine *e);
bool             engine_grow_hist(FinancialEngine *e);

/* ── -6.2  Data Population ── */
void             populate_default_currencies(FinancialEngine *e);
bool             cache_load(FinancialEngine *e);
bool             cache_save(const FinancialEngine *e);
bool             config_load(FinancialEngine *e);
bool             config_save(const FinancialEngine *e);

/* ── -6.3  Core Conversion Engine ── */
double   conv_basic(const FinancialEngine *e, const char *from,
                    const char *to, double amount);
double   conv_markup(const FinancialEngine *e, const char *from,
                     const char *to, double amount, double markup_pct);
double   conv_with_tax(const FinancialEngine *e, const char *from,
                       const char *to, double amount,
                       double markup_pct, double tax_pct);
double   conv_cross_rate(const FinancialEngine *e, const char *from,
                         const char *to, double amount);
double   conv_inverse_rate(const FinancialEngine *e,
                           const char *from, const char *to);
double   conv_apply_rounding(double value, RoundingMode mode, int prec);
void     conv_bulk(const FinancialEngine *e, const char *from,
                   const char **targets, int n, double amount);
void     conv_all_to_all(const FinancialEngine *e, const char *from,
                          double amount);

/* ── -6.4  Currency Lookup / Search ── */
Currency        *db_find(const FinancialEngine *e, const char *code);
int              db_find_idx(const FinancialEngine *e, const char *code);
void             db_auto_suggest(const FinancialEngine *e, const char *partial);

/* ── -6.5  Sorting & Filtering ── */
void  db_sort(FinancialEngine *e, SortField field, SortOrder order);
void  db_filter(const FinancialEngine *e, const char *kw);

/* ── -6.6  Financial Tools ── */
double  fin_inflation(double principal, double rate_pct, int years);
double  fin_goal(const FinancialEngine *e,
                 const char *local, const char *foreign, double target);
void    fin_travel_budget(const FinancialEngine *e, const char *home,
                          const char *dest, const double *amounts,
                          const char *descs[], int n);
double  fin_salary_ppp(const FinancialEngine *e,
                       const char *from, const char *to, double salary);
double  fin_tax_calc(double amount, double rate_pct);
void    fin_pivot_points(const FinancialEngine *e, const char *code);
double  fin_sma(const double *rates, int n);
double  fin_volatility(const FinancialEngine *e, const char *code);
void    fin_compare(const FinancialEngine *e,
                    const char *c1, const char *c2);
void    fin_top_gainers(const FinancialEngine *e, int n);
void    fin_top_losers(const FinancialEngine *e, int n);
void    fin_percentage_change(const FinancialEngine *e, const char *code);
void    fin_avg_rate(const FinancialEngine *e, const char *code);
void    fin_market_data(const FinancialEngine *e, const char *code);

/* ── -6.7  Wallet ── */
bool    wallet_deposit(FinancialEngine *e, const char *code, double amount);
bool    wallet_withdraw(FinancialEngine *e, const char *code, double amount);
void    wallet_show(const FinancialEngine *e);
double  wallet_total_usd(const FinancialEngine *e);
void    wallet_distribution(const FinancialEngine *e);

/* ── -6.8  Favorites ── */
bool    fav_add(FinancialEngine *e, const char *code);
bool    fav_remove(FinancialEngine *e, const char *code);
void    fav_show(const FinancialEngine *e);

/* ── -6.9  Price Alerts ── */
bool    alert_add(FinancialEngine *e, const char *code,
                  double target, bool above);
void    alert_check(FinancialEngine *e);
void    alert_list(const FinancialEngine *e);

/* ── -6.10  History ── */
bool    hist_push(FinancialEngine *e, const ConversionRecord *r);
void    hist_show(const FinancialEngine *e, int last_n);
void    hist_clear(FinancialEngine *e);

/* ── -6.11  File I/O ── */
bool    io_export_csv(const FinancialEngine *e);
bool    io_backup(const FinancialEngine *e);
bool    io_restore(FinancialEngine *e);
bool    io_log_conversion(const FinancialEngine *e, const ConversionRecord *r);
void    io_write_log(const FinancialEngine *e, LogLevel lvl,
                     const char *fmt, ...);
bool    io_save_wallet(const FinancialEngine *e);
bool    io_load_wallet(FinancialEngine *e);

/* ── -6.12  Display / UI ── */
void    ui_header(const FinancialEngine *e);
void    ui_main_menu(const FinancialEngine *e);
void    ui_conversion_table(const FinancialEngine *e, const char *from,
                             double amount, const char **targets,
                             int n, const double *results);
void    ui_currency_card(const FinancialEngine *e, const char *code);
void    ui_history(const FinancialEngine *e, int last_n);
void    ui_mini_graph(const double *vals, int n, const char *label,
                      const char *color);
void    ui_progress_bar(int cur, int total, int bar_w, const char *label);
void    ui_about(void);
void    ui_tutorial(void);
void    ui_system_info(void);
void    ui_separator(int w, const char *color, char ch);
void    ui_box_line(char left, char fill, char right, int w,
                    const char *color);
void    ui_news_feed(void);
void    clear_screen(void);
void    beep(bool is_error);
void    anim_loading(int ms, const char *label);
void    anim_binary_rain(int ms);
void    anim_spinner(int steps, const char *label);

/* ── -6.13  Formatting ── */
void        fmt_number(char *buf, size_t sz, double v,
                       int prec, bool sci);
void        fmt_currency_val(char *buf, size_t sz, const Currency *c,
                              double v, int prec);
const char *fmt_change_color(double change);
void        fmt_timestamp(char *buf, size_t sz, time_t t);
const char *get_symbol_str(const FinancialEngine *e, const char *code);

/* ── -6.14  Security ── */
bool    sec_validate_amount(const char *s, double *out);
bool    sec_validate_code(const char *code);
void    sec_sanitize(char *s, size_t maxlen);
bool    sec_setup_pin(FinancialEngine *e);
bool    sec_verify_pin(const FinancialEngine *e);
void    sec_mask_key(const char *key, char *out, size_t len);
void    sec_wipe(void *ptr, size_t sz);
bool    sec_check_checksum(const FinancialEngine *e);

/* ── -6.15  Automation / Notifications ── */
void    auto_startup_update(FinancialEngine *e);
void    auto_daily_report(const FinancialEngine *e);
bool    auto_inactivity_check(const FinancialEngine *e);

/* ── -6.16  Unit Converter (bonus tool) ── */
double  uc_length(double v, const char *from, const char *to);
double  uc_weight(double v, const char *from, const char *to);
void    uc_show_menu(void);

/* ── -6.17  Easter Eggs / Fun ── */
void    ee_fortune_cookie(void);
void    ee_currency_trivia(void);
void    ee_guess_the_rate(FinancialEngine *e);
void    ee_hacker_mode(FinancialEngine *e);
void    ee_time_travel(const FinancialEngine *e, const char *from,
                       const char *to, double amount, int year);
void    ee_space_mode(const FinancialEngine *e,
                      const char *code, double amount);
void    ee_donation(void);
void    ee_master_badge(void);

/* ── -6.18  Command-Line Interface ── */
void    cli_parse(FinancialEngine *e, const char *raw);
void    cli_run_argv(FinancialEngine *e, int argc, char *argv[]);

/* ── -6.19  i18n — Translation ── */
const char *i18n(const FinancialEngine *e, const char *key);

/* ── -6.20  Misc helpers ── */
void    touch_activity(FinancialEngine *e);
void    badge_check(FinancialEngine *e);
void    print_c(const char *color, const char *fmt, ...);

#endif /* CONVERTER_H */
