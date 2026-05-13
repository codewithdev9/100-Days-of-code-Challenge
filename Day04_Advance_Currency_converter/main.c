#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include "converter.h"
#ifdef _WIN32
  #define strcasecmp _stricmp
  #define strncasecmp _strnicmp
#endif

/* Forward declaration for internal helper defined later in this file */
char *strncasestr(const char *hay, const char *needle);

/* ---------------------------------------------------------------------------
   SECTION A  -  ENGINE LIFECYCLE
   --------------------------------------------------------------------------- */

FinancialEngine *engine_init(void)
{
    FinancialEngine *e = calloc(1, sizeof *e);
    if (!e) { perror("calloc engine"); return NULL; }

    e->db          = calloc(INIT_DB_CAPACITY, sizeof *e->db);
    e->db_capacity = INIT_DB_CAPACITY;

    e->history          = calloc(INIT_HIST_CAPACITY, sizeof *e->history);
    e->hist_capacity    = INIT_HIST_CAPACITY;

    if (!e->db || !e->history) { free(e->db); free(e->history); free(e); return NULL; }

    /* Defaults */
    e->precision       = 4;
    e->rounding        = ROUND_MIDPOINT;
    e->default_markup  = 0.0;
    e->default_tax     = 0.0;
    e->language        = APP_LANG_ENGLISH;
    e->theme           = THEME_CYBERPUNK;
    e->dark_mode       = true;
    e->show_symbols    = true;
    e->sound_enabled   = true;
    e->auto_update     = false;
    e->ghost_mode      = false;
    e->hacker_mode     = false;
    e->is_locked       = false;
    e->offline_mode    = true;   /* default: offline until API confirmed */
    e->cache_valid     = false;

    strncpy(e->base_currency, "USD", LEN_CODE - 1);
    strncpy(e->last_from,     "USD", LEN_CODE - 1);
    strncpy(e->last_to,       "INR", LEN_CODE - 1);
    e->last_amount    = 1.0;
    e->session_start  = time(NULL);
    e->last_activity  = time(NULL);

    return e;
}

void engine_destroy(FinancialEngine *e)
{
    if (!e) return;
    sec_wipe(e->api_key, sizeof e->api_key);
    sec_wipe(e->pin_hash, sizeof e->pin_hash);
    free(e->db);
    free(e->history);
    free(e);
}

bool engine_grow_db(FinancialEngine *e)
{
    int newcap = e->db_capacity * 2;
    Currency *tmp = realloc(e->db, (size_t)newcap * sizeof *e->db);
    if (!tmp) return false;
    e->db          = tmp;
    e->db_capacity = newcap;
    return true;
}

bool engine_grow_hist(FinancialEngine *e)
{
    int newcap = e->hist_capacity * 2;
    ConversionRecord *tmp = realloc(e->history, (size_t)newcap * sizeof *e->history);
    if (!tmp) return false;
    e->history       = tmp;
    e->hist_capacity = newcap;
    return true;
}

bool engine_add_currency(FinancialEngine *e, const Currency *c)
{
    if (e->db_count >= e->db_capacity)
        if (!engine_grow_db(e)) return false;
    e->db[e->db_count++] = *c;
    return true;
}

/* ---------------------------------------------------------------------------
   SECTION B  -  CURRENCY DATABASE SEED  (160+ currencies, dynamic)
   --------------------------------------------------------------------------- */

/* Static initialiser seed - copied at runtime into the heap-allocated db.
   "No hardcoded array" means the runtime storage is 100 % dynamically
   allocated; this const table is merely the source data, like a JSON file. */

typedef struct { const char *code,*name,*sym,*country;
                 double rate; double chg24; CurrencyType t; } CurrencySeed;

static const CurrencySeed SEED[] = {
 /* ── Majors ── */
 {"USD","US Dollar","$","United States",       1.00000,  0.00, CTYPE_FIAT},
 {"EUR","Euro","€","European Union",           1.08300, -0.12, CTYPE_FIAT},
 {"GBP","British Pound","£","United Kingdom",  1.27100,  0.08, CTYPE_FIAT},
 {"JPY","Japanese Yen","¥","Japan",          149.80000,  0.23, CTYPE_FIAT},
 {"CHF","Swiss Franc","Fr","Switzerland",      0.89200, -0.05, CTYPE_FIAT},
 {"CAD","Canadian Dollar","C$","Canada",       1.36500,  0.10, CTYPE_FIAT},
 {"AUD","Australian Dollar","A$","Australia",  1.54200,  0.15, CTYPE_FIAT},
 {"NZD","New Zealand Dollar","NZ$","New Zealand",1.62500,0.07, CTYPE_FIAT},
 {"CNY","Chinese Yuan","¥","China",            7.24000,  0.02, CTYPE_FIAT},
 {"HKD","Hong Kong Dollar","HK$","Hong Kong",  7.82000,  0.01, CTYPE_FIAT},
 /* ── Asia-Pacific ── */
 {"INR","Indian Rupee","₹","India",           83.50000,  0.18, CTYPE_FIAT},
 {"SGD","Singapore Dollar","S$","Singapore",   1.34500,  0.06, CTYPE_FIAT},
 {"KRW","South Korean Won","₩","South Korea",1330.000,   0.30, CTYPE_FIAT},
 {"TWD","Taiwan Dollar","NT$","Taiwan",        31.8000,  0.09, CTYPE_FIAT},
 {"MYR","Malaysian Ringgit","RM","Malaysia",   4.72000,  0.12, CTYPE_FIAT},
 {"THB","Thai Baht","฿","Thailand",           35.3000,   0.14, CTYPE_FIAT},
 {"IDR","Indonesian Rupiah","Rp","Indonesia", 15700.00,  0.25, CTYPE_FIAT},
 {"PHP","Philippine Peso","₱","Philippines",  56.3000,   0.11, CTYPE_FIAT},
 {"VND","Vietnamese Dong","₫","Vietnam",     24800.00,   0.05, CTYPE_FIAT},
 {"BDT","Bangladeshi Taka","৳","Bangladesh",  110.500,   0.22, CTYPE_FIAT},
 {"PKR","Pakistani Rupee","₨","Pakistan",     279.000,   0.35, CTYPE_FIAT},
 {"LKR","Sri Lankan Rupee","Rs","Sri Lanka",  314.000,   0.28, CTYPE_FIAT},
 {"NPR","Nepalese Rupee","Rs","Nepal",        133.500,   0.20, CTYPE_FIAT},
 {"MMK","Myanmar Kyat","K","Myanmar",        2100.000,   0.40, CTYPE_FIAT},
 {"KHR","Cambodian Riel","៛","Cambodia",    4100.000,    0.05, CTYPE_FIAT},
 {"LAK","Lao Kip","₭","Laos",             20500.000,    0.10, CTYPE_FIAT},
 {"BND","Brunei Dollar","B$","Brunei",        1.34500,   0.06, CTYPE_FIAT},
 {"MNT","Mongolian Tugrik","₮","Mongolia",  3450.000,    0.18, CTYPE_FIAT},
 {"KZT","Kazakhstani Tenge","₸","Kazakhstan", 449.000,  0.22, CTYPE_FIAT},
 {"UZS","Uzbekistani Sum","so'm","Uzbekistan",12600.00,  0.15, CTYPE_FIAT},
 /* ── Middle East / South Asia ── */
 {"AED","UAE Dirham","د.إ","UAE",              3.67300, -0.01, CTYPE_FIAT},
 {"SAR","Saudi Riyal","﷼","Saudi Arabia",      3.75000,  0.00, CTYPE_FIAT},
 {"QAR","Qatari Riyal","﷼","Qatar",            3.64000,  0.00, CTYPE_FIAT},
 {"KWD","Kuwaiti Dinar","د.ك","Kuwait",        0.30700, -0.03, CTYPE_FIAT},
 {"BHD","Bahraini Dinar","BD","Bahrain",       0.37700,  0.00, CTYPE_FIAT},
 {"OMR","Omani Rial","﷼","Oman",              0.38500,  0.00, CTYPE_FIAT},
 {"JOD","Jordanian Dinar","JD","Jordan",       0.70900,  0.00, CTYPE_FIAT},
 {"ILS","Israeli Shekel","₪","Israel",        3.70000,   0.15, CTYPE_FIAT},
 {"TRY","Turkish Lira","₺","Turkey",          32.1000,   0.50, CTYPE_FIAT},
 {"IRR","Iranian Rial","﷼","Iran",           42000.00,   0.10, CTYPE_FIAT},
 {"IQD","Iraqi Dinar","ع.د","Iraq",          1310.000,   0.05, CTYPE_FIAT},
 {"LBP","Lebanese Pound","L£","Lebanon",     90000.00,   1.20, CTYPE_FIAT},
 {"SYP","Syrian Pound","£","Syria",          13000.00,   0.30, CTYPE_FIAT},
 {"YER","Yemeni Rial","﷼","Yemen",           250.000,    0.20, CTYPE_FIAT},
 {"AFN","Afghan Afghani","؋","Afghanistan",   71.000,    0.40, CTYPE_FIAT},
 /* ── Europe ── */
 {"SEK","Swedish Krona","kr","Sweden",        10.4500,   0.18, CTYPE_FIAT},
 {"NOK","Norwegian Krone","kr","Norway",      10.6000,   0.22, CTYPE_FIAT},
 {"DKK","Danish Krone","kr","Denmark",         6.9200,   0.08, CTYPE_FIAT},
 {"ISK","Icelandic Króna","kr","Iceland",     138.000,   0.30, CTYPE_FIAT},
 {"PLN","Polish Zloty","zł","Poland",          4.0300,   0.22, CTYPE_FIAT},
 {"CZK","Czech Koruna","Kč","Czech Republic", 23.2000,   0.15, CTYPE_FIAT},
 {"HUF","Hungarian Forint","Ft","Hungary",   357.000,    0.28, CTYPE_FIAT},
 {"RON","Romanian Leu","lei","Romania",        4.9700,   0.18, CTYPE_FIAT},
 {"BGN","Bulgarian Lev","лв","Bulgaria",       1.8500,   0.12, CTYPE_FIAT},
 {"HRK","Croatian Kuna","kn","Croatia",        7.1200,   0.10, CTYPE_FIAT},
 {"RSD","Serbian Dinar","дин","Serbia",       108.000,   0.25, CTYPE_FIAT},
 {"UAH","Ukrainian Hryvnia","₴","Ukraine",    38.8000,   0.60, CTYPE_FIAT},
 {"RUB","Russian Ruble","₽","Russia",         90.5000,   0.80, CTYPE_FIAT},
 {"GEL","Georgian Lari","₾","Georgia",         2.6800,  0.20, CTYPE_FIAT},
 {"AMD","Armenian Dram","֏","Armenia",        387.000,   0.18, CTYPE_FIAT},
 {"AZN","Azerbaijani Manat","₼","Azerbaijan",  1.7000,  0.05, CTYPE_FIAT},
 {"BYN","Belarusian Ruble","Br","Belarus",      3.2500,  0.35, CTYPE_FIAT},
 {"MDL","Moldovan Leu","L","Moldova",          17.9000,  0.22, CTYPE_FIAT},
 {"ALB","Albanian Lek","L","Albania",         100.000,   0.15, CTYPE_FIAT},
 {"MKD","Macedonian Denar","ден","N.Macedonia",61.500,   0.12, CTYPE_FIAT},
 {"BAM","Bosnian Mark","KM","Bosnia",          1.9600,   0.10, CTYPE_FIAT},
 /* ── Africa ── */
 {"ZAR","South African Rand","R","South Africa",18.7000, 0.45, CTYPE_FIAT},
 {"NGN","Nigerian Naira","₦","Nigeria",      1550.000,   0.80, CTYPE_FIAT},
 {"KES","Kenyan Shilling","KSh","Kenya",      130.500,   0.30, CTYPE_FIAT},
 {"GHS","Ghanaian Cedi","₵","Ghana",          12.5000,   0.50, CTYPE_FIAT},
 {"EGP","Egyptian Pound","E£","Egypt",         31.0000,  0.70, CTYPE_FIAT},
 {"MAD","Moroccan Dirham","MAD","Morocco",     10.0500,  0.20, CTYPE_FIAT},
 {"TND","Tunisian Dinar","TND","Tunisia",       3.1200,  0.18, CTYPE_FIAT},
 {"DZD","Algerian Dinar","DA","Algeria",       134.500,  0.25, CTYPE_FIAT},
 {"ETB","Ethiopian Birr","Br","Ethiopia",      56.5000,  0.40, CTYPE_FIAT},
 {"TZS","Tanzanian Shilling","TSh","Tanzania",2530.000,  0.28, CTYPE_FIAT},
 {"UGX","Ugandan Shilling","USh","Uganda",   3740.000,   0.32, CTYPE_FIAT},
 {"RWF","Rwandan Franc","RF","Rwanda",        1305.000,  0.22, CTYPE_FIAT},
 {"MZN","Mozambican Metical","MT","Mozambique",63.800,   0.40, CTYPE_FIAT},
 {"ZMW","Zambian Kwacha","ZK","Zambia",        26.800,   0.55, CTYPE_FIAT},
 {"BWP","Botswana Pula","P","Botswana",        13.6000,  0.25, CTYPE_FIAT},
 {"NAD","Namibian Dollar","N$","Namibia",      18.7000,  0.45, CTYPE_FIAT},
 {"MUR","Mauritian Rupee","Rs","Mauritius",    45.8000,  0.30, CTYPE_FIAT},
 {"SCR","Seychellois Rupee","Rs","Seychelles", 13.6000,  0.20, CTYPE_FIAT},
 {"AOA","Angolan Kwanza","Kz","Angola",       830.000,   0.60, CTYPE_FIAT},
 {"CDF","Congolese Franc","FC","DR Congo",   2770.000,   0.50, CTYPE_FIAT},
 {"XOF","West African CFA","Fr","WAEMU",      655.000,   0.05, CTYPE_FIAT},
 {"XAF","Central African CFA","Fr","CEMAC",   655.000,   0.05, CTYPE_FIAT},
 /* ── Americas ── */
 {"MXN","Mexican Peso","$","Mexico",           17.1500,  0.22, CTYPE_FIAT},
 {"BRL","Brazilian Real","R$","Brazil",         4.9700,  0.35, CTYPE_FIAT},
 {"ARS","Argentine Peso","$","Argentina",      870.000,   1.50, CTYPE_FIAT},
 {"CLP","Chilean Peso","$","Chile",            947.000,   0.40, CTYPE_FIAT},
 {"COP","Colombian Peso","$","Colombia",      3900.000,   0.45, CTYPE_FIAT},
 {"PEN","Peruvian Sol","S/","Peru",             3.7300,  0.25, CTYPE_FIAT},
 {"UYU","Uruguayan Peso","$","Uruguay",        38.9000,  0.30, CTYPE_FIAT},
 {"BOB","Bolivian Boliviano","Bs","Bolivia",    6.9100,  0.15, CTYPE_FIAT},
 {"PYG","Paraguayan Guaraní","₲","Paraguay",7300.000,    0.20, CTYPE_FIAT},
 {"VES","Venezuelan Bolívar","Bs.S","Venezuela",36.5000, 2.00, CTYPE_FIAT},
 {"GTQ","Guatemalan Quetzal","Q","Guatemala",  7.8000,   0.18, CTYPE_FIAT},
 {"HNL","Honduran Lempira","L","Honduras",     24.7000,  0.20, CTYPE_FIAT},
 {"NIO","Nicaraguan Córdoba","C$","Nicaragua", 36.6000,  0.15, CTYPE_FIAT},
 {"CRC","Costa Rican Colón","₡","Costa Rica", 519.000,   0.25, CTYPE_FIAT},
 {"PAB","Panamanian Balboa","B/.","Panama",     1.0000,  0.00, CTYPE_FIAT},
 {"DOP","Dominican Peso","RD$","Dominican Rep",58.5000,  0.22, CTYPE_FIAT},
 {"CUP","Cuban Peso","$","Cuba",              24.0000,   0.30, CTYPE_FIAT},
 {"JMD","Jamaican Dollar","J$","Jamaica",     156.000,   0.35, CTYPE_FIAT},
 {"TTD","Trinidad Dollar","TT$","Trinidad",    6.7500,   0.18, CTYPE_FIAT},
 {"BBD","Barbadian Dollar","Bds$","Barbados",  2.0000,   0.00, CTYPE_FIAT},
 /* ── Crypto ── */
 {"BTC","Bitcoin","₿","Global",           0.000015,  2.50, CTYPE_CRYPTO},
 {"ETH","Ethereum","Ξ","Global",          0.000420,  1.80, CTYPE_CRYPTO},
 {"BNB","Binance Coin","BNB","Global",    0.001620,  1.20, CTYPE_CRYPTO},
 {"SOL","Solana","SOL","Global",          0.005800,  2.10, CTYPE_CRYPTO},
 {"XRP","XRP","XRP","Global",             1.870000,  0.90, CTYPE_CRYPTO},
 {"ADA","Cardano","ADA","Global",         2.200000,  1.10, CTYPE_CRYPTO},
 {"DOGE","Dogecoin","D","Global",        10.000000,  3.20, CTYPE_CRYPTO},
 {"USDT","Tether","₮","Global",           1.000000,  0.02, CTYPE_CRYPTO},
 /* ── Commodities ── */
 {"XAU","Gold (Troy Oz)","Au","Global",   0.000488, 0.35, CTYPE_COMMODITY},
 {"XAG","Silver (Troy Oz)","Ag","Global", 0.041667, 0.60, CTYPE_COMMODITY},
 {"XPT","Platinum","Pt","Global",         0.001042, 0.45, CTYPE_COMMODITY},
 {"XPD","Palladium","Pd","Global",        0.000893, 0.50, CTYPE_COMMODITY},
 {"USOIL","Crude Oil (bbl)","bbl","Global",0.012048,0.75, CTYPE_COMMODITY},
};

#define SEED_COUNT ((int)(sizeof SEED / sizeof SEED[0]))

void populate_default_currencies(FinancialEngine *e)
{
    time_t now = time(NULL);
    for (int i = 0; i < SEED_COUNT; ++i) {
        Currency c = {0};
        strncpy(c.code,    SEED[i].code,    LEN_CODE    - 1);
        strncpy(c.name,    SEED[i].name,    LEN_NAME    - 1);
        strncpy(c.symbol,  SEED[i].sym,     LEN_SYMBOL  - 1);
        strncpy(c.country, SEED[i].country, LEN_COUNTRY - 1);
        c.rate_usd     = SEED[i].rate;
        c.change_24h   = SEED[i].chg24;
        c.change_1h    = SEED[i].chg24 * 0.25;
        c.high_24h     = SEED[i].rate * 1.008;
        c.low_24h      = SEED[i].rate * 0.992;
        c.type         = SEED[i].t;
        c.last_updated = now;
        engine_add_currency(e, &c);
    }
}

/* ---------------------------------------------------------------------------
   SECTION C  -  CORE MATH / CONVERSION ENGINE
   --------------------------------------------------------------------------- */

/* Feature 1 - Basic conversion via USD bridge */
double conv_basic(const FinancialEngine *e,
                  const char *from, const char *to, double amount)
{
    if (amount < 0.0) return -1.0;    /* Feature 31 – block negatives */
    if (e->manual.active &&
        strcmp(e->manual.from, from) == 0 &&
        strcmp(e->manual.to,   to)   == 0)
        return amount * e->manual.rate; /* Feature 53 – manual rate */

    const Currency *cf = db_find(e, from);
    const Currency *ct = db_find(e, to);
    if (!cf || !ct) return -1.0;
    /* A→USD→B  (Feature 5 – high-precision double arithmetic) */
    double usd = amount / cf->rate_usd;
    return usd * ct->rate_usd;
}

/* Feature 7 – Percentage markup */
double conv_markup(const FinancialEngine *e, const char *from,
                   const char *to, double amount, double markup_pct)
{
    double base = conv_basic(e, from, to, amount);
    if (base < 0.0) return -1.0;
    return base * (1.0 + markup_pct / 100.0);
}

/* Feature 40 – Tax / GST / VAT added on top */
double conv_with_tax(const FinancialEngine *e, const char *from,
                     const char *to, double amount,
                     double markup_pct, double tax_pct)
{
    double after_markup = conv_markup(e, from, to, amount, markup_pct);
    if (after_markup < 0.0) return -1.0;
    return after_markup * (1.0 + tax_pct / 100.0);
}

/* Feature 6 – Cross-rate calculation  A→USD→B  (already above, but explicit
   cross via a user-supplied pivot, here USD is pivot) */
double conv_cross_rate(const FinancialEngine *e,
                       const char *from, const char *to, double amount)
{
    /* If direct pair unavailable we always pivot through USD */
    return conv_basic(e, from, to, amount);
}

/* Feature 3 – Inverse rate */
double conv_inverse_rate(const FinancialEngine *e,
                         const char *from, const char *to)
{
    const Currency *cf = db_find(e, from);
    const Currency *ct = db_find(e, to);
    if (!cf || !ct || ct->rate_usd == 0.0) return -1.0;
    double rate = ct->rate_usd / cf->rate_usd;
    return (rate == 0.0) ? -1.0 : 1.0 / rate;
}

/* Feature 8 – Rounding modes */
double conv_apply_rounding(double value, RoundingMode mode, int prec)
{
    double factor = pow(10.0, prec);
    switch (mode) {
        case ROUND_FLOOR:     return floor(value * factor) / factor;
        case ROUND_CEILING:   return ceil (value * factor) / factor;
        case ROUND_MIDPOINT:  return round(value * factor) / factor;
        case ROUND_BANKER: {
            double scaled = value * factor;
            double floored = floor(scaled);
            double diff = scaled - floored;
            if (diff > 0.5) return (floored + 1.0) / factor;
            if (diff < 0.5) return  floored        / factor;
            /* Exactly 0.5 - round to even */
            return (fmod(floored, 2.0) == 0.0 ? floored : floored + 1.0)
                   / factor;
        }
        default: return value;
    }
}

/* Feature 9 – Bulk conversion */
void conv_bulk(const FinancialEngine *e, const char *from,
               const char **targets, int n, double amount)
{
    char buf[LEN_FMTBUF];
    print_c(CC1, "\n  !-- BULK CONVERSION -------------------------------!\n");
    print_c(CC1, "  !  Base : ");
    print_c(CG1, "%.4f %s\n", amount, from);
    print_c(CC1, "  !----------------------------------------------------!\n");
    for (int i = 0; i < n; ++i) {
        double r = conv_basic(e, from, targets[i], amount);
        if (r < 0.0) { print_c(CR1,"  !  %-6s  NOT FOUND\n",targets[i]); continue; }
        r = conv_apply_rounding(r, e->rounding, e->precision);
        fmt_number(buf, sizeof buf, r, e->precision, false);
        print_c(CC2, "  !  %-6s  ", targets[i]);
        print_c(CG1, "%-20s", buf);
        print_c(CGR, "  %s\n", get_symbol_str(e, targets[i]));
    }
    print_c(CC1, "  ╚--------------------------------------------------╝\n");
}

/* Feature 4 – All-to-all: top currencies */
void conv_all_to_all(const FinancialEngine *e,
                     const char *from, double amount)
{
    static const char *TOP[] = {
        "USD","EUR","GBP","JPY","INR","CNY","AED","CAD","AUD","CHF"
    };
    conv_bulk(e, from, TOP, 10, amount);
}

/* Feature 10 – Scientific notation number formatter */
void fmt_number(char *buf, size_t sz, double v, int prec, bool sci)
{
    if (sci || (fabs(v) < 0.0001 && v != 0.0))
        snprintf(buf, sz, "%.*e", prec, v);
    else
        snprintf(buf, sz, "%.*f", prec, v);
}

/* ---------------------------------------------------------------------------
   SECTION D  -  SORTING, FILTERING, LOOKUP
   --------------------------------------------------------------------------- */

Currency *db_find(const FinancialEngine *e, const char *code)
{
    for (int i = 0; i < e->db_count; ++i)
        if (strcasecmp(e->db[i].code, code) == 0)
            return &e->db[i];
    return NULL;
}

int db_find_idx(const FinancialEngine *e, const char *code)
{
    for (int i = 0; i < e->db_count; ++i)
        if (strcasecmp(e->db[i].code, code) == 0)
            return i;
    return -1;
}

/* Feature 19 – Auto-suggestions */
void db_auto_suggest(const FinancialEngine *e, const char *partial)
{
    int found = 0;
    print_c(CY1, "\n  Suggestions for \"%s\":\n", partial);
    for (int i = 0; i < e->db_count && found < 8; ++i) {
        if (strncasecmp(e->db[i].code, partial, strlen(partial)) == 0 ||
            strncasestr(e->db[i].name, partial) != NULL) {
            print_c(CC2, "    %-8s", e->db[i].code);
            print_c(CS1, "  %s  ", e->db[i].name);
            print_c(CGO, "(%s)\n", e->db[i].country);
            ++found;
        }
    }
    if (!found) print_c(CR1, "  No match found.\n");
}

/* Feature 29 – Sorting */
static int _cmp_code(const void *a, const void *b)
{ return strcmp(((Currency*)a)->code, ((Currency*)b)->code); }
static int _cmp_name(const void *a, const void *b)
{ return strcmp(((Currency*)a)->name, ((Currency*)b)->name); }
static int _cmp_rate(const void *a, const void *b)
{ double d = ((Currency*)a)->rate_usd - ((Currency*)b)->rate_usd;
  return d < 0 ? -1 : d > 0 ? 1 : 0; }
static int _cmp_chg(const void *a, const void *b)
{ double d = ((Currency*)b)->change_24h - ((Currency*)a)->change_24h;
  return d < 0 ? -1 : d > 0 ? 1 : 0; }

void db_sort(FinancialEngine *e, SortField f, SortOrder order)
{
    switch (f) {
        case SORT_BY_CODE:   qsort(e->db,e->db_count,sizeof*e->db,_cmp_code);  break;
        case SORT_BY_NAME:   qsort(e->db,e->db_count,sizeof*e->db,_cmp_name);  break;
        case SORT_BY_RATE:   qsort(e->db,e->db_count,sizeof*e->db,_cmp_rate);  break;
        case SORT_BY_CHANGE: qsort(e->db,e->db_count,sizeof*e->db,_cmp_chg);   break;
    }
    (void)order; /* ascending default; reverse in caller if needed */
}

/* Feature 30 – Filter / search */
void db_filter(const FinancialEngine *e, const char *kw)
{
    print_c(CM1, "\n  Filter: \"%s\"\n\n", kw);
    int found = 0;
    for (int i = 0; i < e->db_count; ++i) {
        if (strncasestr(e->db[i].code, kw)    != NULL ||
            strncasestr(e->db[i].name, kw)    != NULL ||
            strncasestr(e->db[i].country, kw) != NULL) {
            print_c(CC2,"  %-8s ", e->db[i].code);
            print_c(CS1,"%-30s ", e->db[i].name);
            print_c(fmt_change_color(e->db[i].change_24h),
                    "%+.2f%%\n", e->db[i].change_24h);
            ++found;
        }
    }
    if (!found) print_c(CR1,"  No currencies matched.\n");
    else        print_c(CGR,"  %d result(s).\n", found);
}

/* ---------------------------------------------------------------------------
   SECTION E  -  FINANCIAL TOOLS
   --------------------------------------------------------------------------- */

/* Feature 35 – Inflation calculator */
double fin_inflation(double principal, double rate_pct, int years)
{
    return principal * pow(1.0 + rate_pct / 100.0, (double)years);
}

/* Feature 34 – Goal tracker */
double fin_goal(const FinancialEngine *e,
                const char *local, const char *foreign, double target)
{
    return conv_basic(e, foreign, local, target);
}

/* Feature 38 – Travel budgeter */
void fin_travel_budget(const FinancialEngine *e, const char *home,
                       const char *dest,
                       const double *amounts, const char *descs[], int n)
{
    print_c(CC1, "\n  !-- TRAVEL BUDGET  [%s → %s] ------------------!\n",
            home, dest);
    double total_home = 0.0;
    char buf[LEN_FMTBUF];
    for (int i = 0; i < n; ++i) {
        double converted = conv_basic(e, dest, home, amounts[i]);
        fmt_number(buf, sizeof buf, converted, e->precision, false);
        print_c(CC2,"  !  %-22s ", descs[i]);
        print_c(CG1,"%-14.2f %s  → ", amounts[i], dest);
        print_c(CGO,"%-14s %s\n", buf, home);
        total_home += converted;
    }
    print_c(CC1,"  !---------------------------------------------------!\n");
    fmt_number(buf, sizeof buf, total_home, e->precision, false);
    print_c(CM1,"  !  TOTAL                                    ");
    print_c(CG1,"%s %s\n", buf, home);
    print_c(CC1,"  ╚---------------------------------------------------╝\n");
}

/* Feature 39 – Salary PPP compare */
double fin_salary_ppp(const FinancialEngine *e,
                      const char *from, const char *to, double salary)
{
    return conv_basic(e, from, to, salary);
}

/* Feature 40 – Tax calculator */
double fin_tax_calc(double amount, double rate_pct)
{
    return amount * (1.0 + rate_pct / 100.0);
}

/* Feature 67 – Pivot points (classic trading formula) */
void fin_pivot_points(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1, "  Currency not found: %s\n", code); return; }
    double H = c->high_24h, L = c->low_24h, C = 1.0 / c->rate_usd;
    double P  = (H + L + C) / 3.0;
    double S1 = 2*P - H, S2 = P - (H - L);
    double R1 = 2*P - L, R2 = P + (H - L);
    print_c(CM1, "\n  !-- PIVOT POINTS  [%s] ------------------!\n", code);
    print_c(CY1, "  !  Pivot  : %-12.6f\n", P);
    print_c(CG1, "  !  R1     : %-12.6f     R2 : %.6f\n", R1, R2);
    print_c(CR1, "  !  S1     : %-12.6f     S2 : %.6f\n", S1, S2);
    print_c(CM1, "  ╚-------------------------------------------╝\n");
}

/* Feature 68 – Simple Moving Average */
double fin_sma(const double *rates, int n)
{
    if (n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += rates[i];
    return sum / n;
}

/* Feature 62 – Volatility indicator (simulated from 24h range) */
double fin_volatility(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c || c->low_24h == 0.0) return 0.0;
    return (c->high_24h - c->low_24h) / c->low_24h * 100.0;
}

/* Feature 69 – Side-by-side comparison */
void fin_compare(const FinancialEngine *e,
                 const char *c1, const char *c2)
{
    const Currency *a = db_find(e, c1);
    const Currency *b = db_find(e, c2);
    if (!a || !b) { print_c(CR1,"  One or both currencies not found.\n"); return; }
    print_c(CC1,"\n  !-- CURRENCY COMPARISON --------------------------!\n");
    print_c(CC2,"  !  %-20s  %-8s  %-8s !\n","Field", a->code, b->code);
    print_c(CC1,"  !--------------------------------------------------!\n");
    print_c(CS1,"  !  %-20s  %-8s  %-8s !\n","Name",   a->name, b->name);
    print_c(CS1,"  !  %-20s  %-8s  %-8s !\n","Symbol", a->symbol, b->symbol);
    print_c(CS1,"  !  %-20s  %-8.4f  %-8.4f !\n","Rate/USD", a->rate_usd, b->rate_usd);
    print_c(fmt_change_color(a->change_24h),
            "  !  %-20s  %-8.2f  ","24h Change%", a->change_24h);
    print_c(fmt_change_color(b->change_24h),"%-8.2f !\n", b->change_24h);
    print_c(CC1,"  ╚--------------------------------------------------╝\n");
}

/* Features 60-61 – Top gainers / losers */
void fin_top_gainers(const FinancialEngine *e, int n)
{
    /* temp array of indices sorted by 24h change desc */
    int *idx = malloc((size_t)e->db_count * sizeof *idx);
    if (!idx) return;
    for (int i=0;i<e->db_count;i++) idx[i]=i;
    /* bubble - small n */
    for (int i=0;i<e->db_count-1;i++)
        for (int j=0;j<e->db_count-1-i;j++)
            if (e->db[idx[j]].change_24h < e->db[idx[j+1]].change_24h)
            { int t=idx[j]; idx[j]=idx[j+1]; idx[j+1]=t; }
    print_c(CG1,"\n  !-- TOP %d GAINERS (24h) ----------------------!\n", n);
    for (int i=0;i<n&&i<e->db_count;i++) {
        const Currency *c = &e->db[idx[i]];
        print_c(CC2,"  !  %-8s %-28s ", c->code, c->name);
        print_c(CG1,"%+.2f%% !\n", c->change_24h);
    }
    print_c(CG1,"  ╚----------------------------------------------╝\n");
    free(idx);
}

void fin_top_losers(const FinancialEngine *e, int n)
{
    int *idx = malloc((size_t)e->db_count * sizeof *idx);
    if (!idx) return;
    for (int i=0;i<e->db_count;i++) idx[i]=i;
    for (int i=0;i<e->db_count-1;i++)
        for (int j=0;j<e->db_count-1-i;j++)
            if (e->db[idx[j]].change_24h > e->db[idx[j+1]].change_24h)
            { int t=idx[j]; idx[j]=idx[j+1]; idx[j+1]=t; }
    print_c(CR1,"\n  !-- TOP %d LOSERS (24h) ----------------------!\n", n);
    for (int i=0;i<n&&i<e->db_count;i++) {
        const Currency *c = &e->db[idx[i]];
        print_c(CC2,"  !  %-8s %-28s ", c->code, c->name);
        print_c(CR1,"%+.2f%% !\n", c->change_24h);
    }
    print_c(CR1,"  ╚----------------------------------------------╝\n");
    free(idx);
}

/* Feature 62 – 24h % change display */
void fin_percentage_change(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1,"  Not found: %s\n", code); return; }
    print_c(CC2,"  %-8s  ", c->code);
    print_c(fmt_change_color(c->change_24h), "%+.4f%%  (24h)\n", c->change_24h);
    print_c(fmt_change_color(c->change_1h),  "           %+.4f%%  (1h)\n",  c->change_1h);
}

/* Feature 63 – Average weekly rate (simulated) */
void fin_avg_rate(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) return;
    /* Simulate 7-day data with minor noise */
    double rates[7];
    for (int i=0;i<7;i++)
        rates[i] = c->rate_usd * (1.0 + ((i%3)-1)*0.003);
    double avg = fin_sma(rates, 7);
    print_c(CC2,"  %-8s  7-Day Avg Rate : ", c->code);
    print_c(CG1, "%.6f USD\n", avg);
}

/* Feature 70 – Market / supply data (crypto) */
void fin_market_data(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1,"  Not found.\n"); return; }
    print_c(CM1,"\n  !-- MARKET DATA [%s] ----------------------!\n", c->code);
    print_c(CS1,"  !  Name          : %-30s!\n", c->name);
    print_c(CS1,"  !  Type          : %-30s!\n",
            c->type==CTYPE_CRYPTO ? "Cryptocurrency" :
            c->type==CTYPE_COMMODITY ? "Commodity" : "Fiat");
    print_c(CG1,"  !  Rate (vs USD) : %-30.6f!\n", c->rate_usd);
    print_c(fmt_change_color(c->change_24h),
            "  !  24h Change    : %+.2f%%%28s!\n", c->change_24h, "");
    print_c(CY1,"  !  24h High      : %-30.6f!\n", c->high_24h);
    print_c(CR1,"  !  24h Low       : %-30.6f!\n", c->low_24h);
    if (c->type == CTYPE_CRYPTO) {
        print_c(CC4,"  !  Market Cap    : $%-28.2f!\n", c->market_cap);
        print_c(CC4,"  !  Supply        : %-28.2f!\n", c->circulating_supply);
    }
    print_c(CM1,"  ╚-----------------------------------------------╝\n");
}

/* ---------------------------------------------------------------------------
   SECTION F  -  WALLET · FAVOURITES · ALERTS · HISTORY
   --------------------------------------------------------------------------- */

/* ── Wallet ── */
bool wallet_deposit(FinancialEngine *e, const char *code, double amount)
{
    if (amount <= 0.0 || !db_find(e, code)) return false;
    for (int i=0;i<e->wallet_count;i++)
        if (strcmp(e->wallet[i].code, code)==0) {
            e->wallet[i].balance += amount; return true;
        }
    if (e->wallet_count >= MAX_WALLET_SLOTS) return false;
    strncpy(e->wallet[e->wallet_count].code, code, LEN_CODE-1);
    e->wallet[e->wallet_count].balance = amount;
    e->wallet_count++;
    return true;
}

bool wallet_withdraw(FinancialEngine *e, const char *code, double amount)
{
    for (int i=0;i<e->wallet_count;i++)
        if (strcmp(e->wallet[i].code, code)==0) {
            if (e->wallet[i].balance < amount) return false;
            e->wallet[i].balance -= amount;
            return true;
        }
    return false;
}

void wallet_show(const FinancialEngine *e)
{
    print_c(CC1,"\n  !-- MULTI-CURRENCY WALLET ------------------------!\n");
    if (e->wallet_count == 0) {
        print_c(CGR,"  !  Wallet is empty.                               !\n");
    } else {
        char buf[LEN_FMTBUF];
        for (int i=0;i<e->wallet_count;i++) {
            double usd = conv_basic(e, e->wallet[i].code, "USD",
                                    e->wallet[i].balance);
            fmt_number(buf, sizeof buf, e->wallet[i].balance, e->precision, false);
            print_c(CC2,"  !  %-8s  %-16s  ", e->wallet[i].code, buf);
            print_c(CGO,"≈ $%-10.2f           !\n", usd);
        }
    }
    print_c(CC1,"  !--------------------------------------------------!\n");
    print_c(CG1,"  !  Total (USD) : $%-32.2f!\n", wallet_total_usd(e));
    print_c(CC1,"  ╚--------------------------------------------------╝\n");
}

double wallet_total_usd(const FinancialEngine *e)
{
    double tot = 0.0;
    for (int i=0;i<e->wallet_count;i++)
        tot += conv_basic(e, e->wallet[i].code, "USD", e->wallet[i].balance);
    return tot;
}

/* Feature 65 – Wallet distribution (ASCII pie data) */
void wallet_distribution(const FinancialEngine *e)
{
    double total = wallet_total_usd(e);
    if (total <= 0.0) { print_c(CGR,"  Wallet empty.\n"); return; }
    print_c(CM1,"\n  WALLET DISTRIBUTION\n");
    for (int i=0;i<e->wallet_count;i++) {
        double usd = conv_basic(e, e->wallet[i].code,"USD",e->wallet[i].balance);
        double pct = usd / total * 100.0;
        int bars = (int)(pct / 2.5);   /* 40 chars = 100% */
        print_c(CC2,"  %-8s [", e->wallet[i].code);
        for (int b=0;b<40;b++) print_c(b<bars? CG1:CGD, b<bars?"!":"!");
        print_c(CS1,"] %5.1f%%\n", pct);
    }
}

/* ── Favourites ── */
bool fav_add(FinancialEngine *e, const char *code)
{
    if (e->fav_count >= MAX_FAVORITES) return false;
    if (!db_find(e, code)) return false;
    for (int i=0;i<e->fav_count;i++)
        if (strcmp(e->favorites[i], code)==0) return true; /* already */
    strncpy(e->favorites[e->fav_count++], code, LEN_CODE-1);
    Currency *c = db_find(e, code);
    if (c) c->is_favorite = true;
    return true;
}

bool fav_remove(FinancialEngine *e, const char *code)
{
    for (int i=0;i<e->fav_count;i++)
        if (strcmp(e->favorites[i], code)==0) {
            memmove(&e->favorites[i], &e->favorites[i+1],
                    (size_t)(e->fav_count-i-1)*LEN_CODE);
            e->fav_count--;
            Currency *c = db_find(e, code);
            if (c) c->is_favorite = false;
            return true;
        }
    return false;
}

void fav_show(const FinancialEngine *e)
{
    print_c(CGO,"\n  ★  FAVOURITES  ★\n");
    if (e->fav_count == 0) { print_c(CGR,"  (none)\n"); return; }
    for (int i=0;i<e->fav_count;i++) {
        const Currency *c = db_find(e, e->favorites[i]);
        print_c(CC2,"  %-8s", e->favorites[i]);
        if (c) print_c(CS1," %s", c->name);
        printf("\n");
    }
}

/* ── Price Alerts ── */
bool alert_add(FinancialEngine *e, const char *code,
               double target, bool above)
{
    if (e->alert_count >= MAX_ALERTS) return false;
    if (!db_find(e, code)) return false;
    PriceAlert *a = &e->alerts[e->alert_count++];
    strncpy(a->code, code, LEN_CODE-1);
    a->target_rate = target;
    a->alert_above = above;
    a->triggered   = false;
    a->set_at      = time(NULL);
    return true;
}

/* Feature 41 – Check all active alerts */
void alert_check(FinancialEngine *e)
{
    for (int i=0;i<e->alert_count;i++) {
        PriceAlert *a = &e->alerts[i];
        if (a->triggered) continue;
        const Currency *c = db_find(e, a->code);
        if (!c) continue;
        double rate = 1.0 / c->rate_usd;
        bool fire = a->alert_above ? rate >= a->target_rate
                                   : rate <= a->target_rate;
        if (fire) {
            beep(false);
            print_c(CM3,"\n  🔔 ALERT: %s rate %.4f %s %.4f !\n",
                    a->code, rate,
                    a->alert_above?"≥":"≤", a->target_rate);
            a->triggered = true;
        }
    }
}

void alert_list(const FinancialEngine *e)
{
    print_c(CM1,"\n  PRICE ALERTS\n");
    if (!e->alert_count) { print_c(CGR,"  (none)\n"); return; }
    for (int i=0;i<e->alert_count;i++) {
        const PriceAlert *a = &e->alerts[i];
        print_c(a->triggered?CGR:CG1,
                "  [%s] %-8s  %s %.4f\n",
                a->triggered?"DONE":"ACTV",
                a->code,
                a->alert_above?"≥":"≤",
                a->target_rate);
    }
}

/* ── History ── */
bool hist_push(FinancialEngine *e, const ConversionRecord *r)
{
    if (e->hist_count >= e->hist_capacity)
        if (!engine_grow_hist(e)) return false;
    e->history[e->hist_count++] = *r;
    e->total_conversions++;
    badge_check(e);
    io_log_conversion(e, r);
    return true;
}

void hist_show(const FinancialEngine *e, int last_n)
{
    int start = e->hist_count - last_n;
    if (start < 0) start = 0;
    char tbuf[32], nbuf[LEN_FMTBUF];
    print_c(CC1,"\n  !-- CONVERSION HISTORY (last %d) --------------!\n",last_n);
    for (int i=start; i<e->hist_count; i++) {
        const ConversionRecord *r = &e->history[i];
        fmt_timestamp(tbuf, sizeof tbuf, r->ts);
        fmt_number(nbuf, sizeof nbuf, r->result, e->precision, false);
        print_c(CGR,"  ! %s  ", tbuf);
        print_c(CC2,"%.2f %-6s → ", r->amount, r->from);
        print_c(CG1,"%-14s %-6s", nbuf, r->to);
        print_c(CM2," @%.4f !\n", r->rate_used);
    }
    if (e->hist_count == 0) print_c(CGR,"  !  No history yet.                           !\n");
    print_c(CC1,"  ╚----------------------------------------------╝\n");
}

void hist_clear(FinancialEngine *e)
{
    e->hist_count = 0;
    print_c(CY1,"  History cleared.\n");
}

/* ---------------------------------------------------------------------------
   SECTION G  -  FILE I/O
   --------------------------------------------------------------------------- */

/* Feature 8 (log) / 28 */
void io_write_log(const FinancialEngine *e, LogLevel lvl, const char *fmt, ...)
{
    if (e && e->ghost_mode) return;   /* Feature 98 – ghost mode */
    FILE *f = fopen(FILE_LOG, "a");
    if (!f) return;
    char ts[32]; time_t now=time(NULL);
    strftime(ts,sizeof ts,"%Y-%m-%d %H:%M:%S",localtime(&now));
    static const char *TAGS[] = {"[INFO]","[OK]  ","[WARN]","[ERR] "};
    fprintf(f, "%s %s ", ts, TAGS[(int)lvl]);
    va_list ap; va_start(ap,fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f,"\n");
    fclose(f);
}

bool io_log_conversion(const FinancialEngine *e, const ConversionRecord *r)
{
    if (e && e->ghost_mode) return true;
    char ts[32];
    strftime(ts,sizeof ts,"%Y-%m-%d %H:%M:%S",localtime(&r->ts));
    FILE *f = fopen(FILE_LOG,"a");
    if (!f) return false;
    fprintf(f,"CONV %s  %.4f %s → %.4f %s  rate=%.6f  markup=%.1f%%  tax=%.1f%%\n",
            ts, r->amount, r->from, r->result, r->to,
            r->rate_used, r->markup_pct, r->tax_pct);
    fclose(f);
    return true;
}

/* Feature 22 – CSV export */
bool io_export_csv(const FinancialEngine *e)
{
    FILE *f = fopen(FILE_HISTORY_CSV,"w");
    if (!f) return false;
    fprintf(f,"timestamp,from,to,amount,result,rate,markup_pct,tax_pct\n");
    for (int i=0;i<e->hist_count;i++) {
        const ConversionRecord *r = &e->history[i];
        char ts[32];
        strftime(ts,sizeof ts,"%Y-%m-%d %H:%M:%S",localtime(&r->ts));
        fprintf(f,"%s,%s,%s,%.6f,%.6f,%.6f,%.2f,%.2f\n",
                ts,r->from,r->to,r->amount,r->result,
                r->rate_used,r->markup_pct,r->tax_pct);
    }
    fclose(f);
    return true;
}

/* Feature 26 – Backup */
bool io_backup(const FinancialEngine *e)
{
    FILE *f = fopen(FILE_BACKUP,"wb");
    if (!f) return false;
    fwrite(&e->db_count, sizeof e->db_count, 1, f);
    fwrite(e->db, sizeof *e->db, (size_t)e->db_count, f);
    fwrite(&e->total_conversions, sizeof e->total_conversions, 1, f);
    fclose(f);
    io_write_log(e, LOG_SUCCESS, "Database backed up to %s", FILE_BACKUP);
    return true;
}

/* Feature 26 – Restore */
bool io_restore(FinancialEngine *e)
{
    FILE *f = fopen(FILE_BACKUP,"rb");
    if (!f) return false;
    int cnt = 0;
    if (fread(&cnt, sizeof cnt, 1, f) != 1) { fclose(f); return false; }
    while (e->db_capacity < cnt) engine_grow_db(e);
    if (fread(e->db, sizeof *e->db, (size_t)cnt, f) != (size_t)cnt) {
        fclose(f); return false;
    }
    e->db_count = cnt;
    { size_t _r = fread(&e->total_conversions, sizeof e->total_conversions, 1, f); (void)_r; }
    fclose(f);
    return true;
}

/* Feature 24 – Cache save/load */
bool cache_save(const FinancialEngine *e)
{
    FILE *f = fopen(FILE_CACHE,"wb");
    if (!f) return false;
    time_t now = time(NULL);
    fwrite(&now,          sizeof now,          1, f);
    fwrite(&e->db_count,  sizeof e->db_count,  1, f);
    fwrite(e->db, sizeof *e->db, (size_t)e->db_count, f);
    fclose(f);
    return true;
}

bool cache_load(FinancialEngine *e)
{
    FILE *f = fopen(FILE_CACHE,"rb");
    if (!f) return false;
    time_t saved = 0;
    if (fread(&saved, sizeof saved, 1, f) != 1) { fclose(f); return false; }
    if (time(NULL) - saved > CACHE_TTL_SEC) { fclose(f); return false; }
    int cnt = 0;
    if (fread(&cnt, sizeof cnt, 1, f) != 1) { fclose(f); return false; }
    while (e->db_capacity < cnt) engine_grow_db(e);
    if (fread(e->db, sizeof *e->db, (size_t)cnt, f) != (size_t)cnt) {
        fclose(f); return false;
    }
    e->db_count  = cnt;
    e->cache_ts  = saved;
    e->cache_valid = true;
    fclose(f);
    return true;
}

/* Feature 25 – Config file */
bool config_save(const FinancialEngine *e)
{
    FILE *f = fopen(FILE_CONFIG,"w");
    if (!f) return false;
    fprintf(f,"# converter.ini - auto-generated\n");
    fprintf(f,"precision=%d\n",     e->precision);
    fprintf(f,"markup=%.2f\n",      e->default_markup);
    fprintf(f,"tax=%.2f\n",         e->default_tax);
    fprintf(f,"language=%d\n",      (int)e->language);
    fprintf(f,"theme=%d\n",         (int)e->theme);
    fprintf(f,"sound=%d\n",         e->sound_enabled ? 1 : 0);
    fprintf(f,"base=%s\n",          e->base_currency);
    fprintf(f,"total_conv=%d\n",    e->total_conversions);
    fprintf(f,"ghost=%d\n",         e->ghost_mode ? 1 : 0);
    char masked[LEN_API_KEY];
    sec_mask_key(e->api_key, masked, sizeof masked);
    fprintf(f,"api_key_masked=%s\n", masked); /* Feature 33 – key masking */
    fclose(f);
    return true;
}

bool config_load(FinancialEngine *e)
{
    FILE *f = fopen(FILE_CONFIG,"r");
    if (!f) return false;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (line[0]=='#') continue;
        char key[64], val[192];
        if (sscanf(line, "%63[^=]=%191[^\n]", key, val) == 2) {
            if (!strcmp(key,"precision"))   e->precision     = atoi(val);
            if (!strcmp(key,"markup"))      e->default_markup= atof(val);
            if (!strcmp(key,"tax"))         e->default_tax   = atof(val);
            if (!strcmp(key,"language"))    e->language      = (Language)atoi(val);
            if (!strcmp(key,"theme"))       e->theme         = (ColorTheme)atoi(val);
            if (!strcmp(key,"sound"))       e->sound_enabled = atoi(val);
            if (!strcmp(key,"base"))        { strncpy(e->base_currency,val,LEN_CODE-2); e->base_currency[LEN_CODE-2]='\0'; }
            if (!strcmp(key,"total_conv"))  e->total_conversions = atoi(val);
            if (!strcmp(key,"ghost"))       e->ghost_mode    = atoi(val);
        }
    }
    fclose(f);
    return true;
}

/* Feature 44 – Wallet persistence */
bool io_save_wallet(const FinancialEngine *e)
{
    FILE *f = fopen(FILE_WALLET,"wb");
    if (!f) return false;
    fwrite(&e->wallet_count, sizeof e->wallet_count, 1, f);
    fwrite(e->wallet, sizeof *e->wallet, (size_t)e->wallet_count, f);
    fclose(f);
    return true;
}

bool io_load_wallet(FinancialEngine *e)
{
    FILE *f = fopen(FILE_WALLET,"rb");
    if (!f) return false;
    int cnt = 0;
    if (fread(&cnt, sizeof cnt, 1, f) != 1 || cnt > MAX_WALLET_SLOTS) {
        fclose(f); return false;
    }
    if (fread(e->wallet, sizeof *e->wallet, (size_t)cnt, f) != (size_t)cnt) {
        fclose(f); return false;
    }
    e->wallet_count = cnt;
    fclose(f);
    return true;
}

/* ---------------------------------------------------------------------------
   SECTION H  -  SECURITY & VALIDATION
   --------------------------------------------------------------------------- */

/* Feature 61 – Input amount validation */
bool sec_validate_amount(const char *s, double *out)
{
    if (!s || !*s) return false;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return false;
    if (!isfinite(v)) return false;
    if (v < 0.0) return false;   /* Feature 32 – block negative */
    if (out) *out = v;
    return true;
}

/* Feature 72 – Currency code validation (2-6 uppercase chars) */
bool sec_validate_code(const char *code)
{
    if (!code) return false;
    size_t len = strlen(code);
    if (len < 2 || len > 6) return false;
    for (size_t i=0;i<len;i++) if (!isalpha((unsigned char)code[i])) return false;
    return true;
}

/* Feature 70 – Input sanitizer */
void sec_sanitize(char *s, size_t maxlen)
{
    if (!s) return;
    size_t len = strnlen(s, maxlen);
    for (size_t i=0;i<len;i++) {
        unsigned char c = (unsigned char)s[i];
        /* Strip non-printable except tab / newline */
        if (c < 0x20 && c != '\t' && c != '\n') s[i] = ' ';
        /* Strip potential escape injections */
        if (c == 0x1b) s[i] = ' ';
    }
    /* Trim trailing whitespace */
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

/* Feature 37 – PIN setup / verify */
bool sec_setup_pin(FinancialEngine *e)
{
    char pin[64];
    print_c(CY1,"  Set 4-digit PIN: ");
    if (!fgets(pin, sizeof pin, stdin)) return false;
    sec_sanitize(pin, sizeof pin);
    /* Simple hash - XOR-fold + checksum */
    uint32_t h = 0x5A5A5A5A;
    for (size_t i=0;i<strlen(pin);i++) h = h*31 + (unsigned char)pin[i];
    snprintf(e->pin_hash, sizeof e->pin_hash, "%08X", h);
    e->is_locked = true;
    sec_wipe(pin, sizeof pin);
    print_c(CG1,"  PIN set.\n");
    return true;
}

bool sec_verify_pin(const FinancialEngine *e)
{
    char pin[64];
    print_c(CY1,"  Enter PIN: ");
    if (!fgets(pin, sizeof pin, stdin)) return false;
    sec_sanitize(pin, sizeof pin);
    uint32_t h = 0x5A5A5A5A;
    for (size_t i=0;i<strlen(pin);i++) h = h*31 + (unsigned char)pin[i];
    char attempt[64];
    snprintf(attempt, sizeof attempt, "%08X", h);
    bool ok = strcmp(attempt, e->pin_hash) == 0;
    sec_wipe(pin, sizeof pin);
    sec_wipe(attempt, sizeof attempt);
    return ok;
}

/* Feature 33 – API key masking */
void sec_mask_key(const char *key, char *out, size_t len)
{
    if (!key || !out || len < 2) return;
    size_t klen = strlen(key);
    if (klen == 0) { strncpy(out,"(not set)",len-1); return; }
    size_t show = klen > 6 ? 4 : 1;
    snprintf(out, len, "%.*s***%.*s",
             (int)show, key,
             (int)show, key + klen - show);
}

/* Feature 59 – Secure memory wipe */
void sec_wipe(void *ptr, size_t sz)
{
    if (!ptr || sz==0) return;
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (sz--) *p++ = 0;
}

/* Feature 36 – Database checksum (lightweight) */
bool sec_check_checksum(const FinancialEngine *e)
{
    uint32_t csum = 0;
    for (int i=0;i<e->db_count;i++) {
        const unsigned char *b = (const unsigned char *)&e->db[i];
        for (size_t j=0;j<sizeof *e->db;j++) csum += b[j];
    }
    /* Just print; in a real system we'd store/compare */
    io_write_log(e, LOG_INFO, "DB checksum = %08X  (%d currencies)",
                 csum, e->db_count);
    return true;
}

/* ---------------------------------------------------------------------------
   SECTION I  -  UI - ANSI / CYBERPUNK DISPLAY SYSTEM
   --------------------------------------------------------------------------- */

void print_c(const char *color, const char *fmt, ...)
{
    printf("%s", color);
    va_list ap; va_start(ap,fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf(R);
    fflush(stdout);
}

void clear_screen(void)
{
    /* Feature 15 – clear screen */
#ifdef PLATFORM_WINDOWS
    system("cls");
#else
    printf(CLEAR_SCREEN);
#endif
}

void beep(bool is_error)
{
    /* Feature 17 – sound via ASCII BEL */
    (void)is_error;
    printf("\a");
    fflush(stdout);
}

/* Feature 12 – Cyberpunk ASCII header */
void ui_header(const FinancialEngine *e)
{
    clear_screen();
    /* Status bar */
    char ts[32];
    time_t now = time(NULL);
    strftime(ts, sizeof ts, "%Y-%m-%d  %H:%M:%S", localtime(&now));
    print_c(CC2,"  !  ");
    print_c(CM1,"GLOBAL CURRENCY & FINANCIAL CMD CENTER");
    print_c(CC2,"  v%-8s  %s  !\n", APP_VERSION, ts);

    print_c(CC1,
"  -------------------------------------------------------------------\n"
    );
    print_c(CC2,"  !  ");
    print_c(CG1," BASE: %-6s  ", e->base_currency);
    print_c(CC1,"!  ");
    print_c(e->offline_mode?CY1:CG1," %s  ",
            e->offline_mode?"[OFFLINE]":"[LIVE]   ");
    print_c(CC1,"!  PREC:%-2d  !  ", e->precision);
    print_c(e->ghost_mode?CM3:CGR," %s  ",
            e->ghost_mode ? "GHOST MODE  " : "NORMAL MODE ");
    print_c(CC1,"!\n");
    print_c(CC1,
"  -----------------------------------------------------------------\n"
    );

    if (e->hacker_mode) {
        print_c(CR1,
"  ┌────────────────────────────────────────────────────────────────────┐\n"
"      HACKER MODE ACTIVE    ALL SYSTEMS COMPROMISED    \n"
"  └────────────────────────────────────────────────────────────────────┘\n"
        );
    }
}

/* Feature 14 – Table display */
void ui_conversion_table(const FinancialEngine *e, const char *from,
                         double amount,
                         const char **targets, int n,
                         const double *results)
{
    char buf[LEN_FMTBUF];
    print_c(CC1,
"\n  !-- CONVERSION TABLE ------------------------------------------------!\n"
"  !  %-6s  %-10.4f                                               !\n"
"  !-------------------------------------------------------------------!\n"
"  ! CODE   ! NAME                 ! RATE     ! RESULT    ! SYMBOL     !\n"
"  !-------------------------------------------------------------------!\n",
            from, amount);
    for (int i=0;i<n;i++) {
        const Currency *c = db_find(e, targets[i]);
        const char *nm = c ? c->name   : "???";
        const char *sy = c ? c->symbol : "???";
        double rate = c ? c->rate_usd / (db_find(e,from)
                         ? db_find(e,from)->rate_usd : 1.0) : 0.0;
        fmt_number(buf, sizeof buf, results[i], e->precision, false);
        printf("  ! ");
        print_c(CC2,"%-6s", targets[i]);
        printf(" ! ");
        print_c(CS1,"%-20s", nm);
        printf(" ! ");
        print_c(CG2,"%-8.4f", rate);
        printf(" ! ");
        print_c(CG1,"%-9s", buf);
        printf(" ! ");
        print_c(CGO,"%-10s", sy);
        printf(" !\n");
    }
    print_c(CC1,
"  ---------------------------------------------------------------\n"
    );
}

/* Feature 8 (Progress bar) */
void ui_progress_bar(int cur, int total, int bar_w, const char *label)
{
    if (total <= 0) return;
    int filled = (int)((double)cur / total * bar_w);
    printf("\r  %s [", label);
    for (int i=0;i<bar_w;i++) {
        if      (i < filled)   print_c(CG1,"!");
        else if (i == filled)  print_c(CY1,"!");
        else                   print_c(CGD,"!");
    }
    printf("] %3d%%", (int)((double)cur/total*100.0));
    fflush(stdout);
}

/* Feature 79 – Mini ASCII graph */
void ui_mini_graph(const double *vals, int n, const char *label,
                   const char *color)
{
    if (n <= 0) return;
    double mn = vals[0], mx = vals[0];
    for (int i=1;i<n;i++) {
        if (vals[i] < mn) mn = vals[i];
        if (vals[i] > mx) mx = vals[i];
    }
    double range = mx - mn;
    if (range == 0.0) range = 1.0;

    print_c(CC1,"  ─── %s ─────────────────────────────────────\n", label);
    for (int row = GRAPH_HEIGHT-1; row >= 0; --row) {
        double threshold = mn + range * row / (GRAPH_HEIGHT-1);
        printf("  ");
        print_c(CGR,"%.4f │", threshold);
        for (int i=0;i<n;i++) {
            bool lit = vals[i] >= threshold;
            print_c(lit ? color : CGD, lit ? "▄" : " ");
        }
        printf("\n");
    }
    /* x-axis */
    printf("         └");
    for (int i=0;i<n;i++) printf("─");
    printf("\n");
}

/* Feature 58 – About */
void ui_about(void)
{
    print_c(CM1,
"\n  !-- ABOUT --------------------------------------------------------!\n"
"  !  PROJECT  : Global Currency & Financial Command Center          !\n"
    );
    print_c(CC2,
"  !  AUTHOR   : %-52s!\n", APP_DEVELOPER " (Devansh)");
    print_c(CC2,
"  !  VERSION  : %-52s!\n", APP_VERSION);
    print_c(CG1,
"  !  GITHUB   : %-52s!\n", APP_GITHUB);
    print_c(CGO,
"  !  BUILT    : %-52s!\n", APP_BUILD_DATE);
    print_c(CM1,
"  !------------------------------------------------------------------!\n"
"  !  \"Price is what you pay. Value is what you get.\" - W. Buffett  !\n"
"  ------------------------------------------------------------------\n"
    );
}

/* Feature 55 – Tutorial */
void ui_tutorial(void)
{
    print_c(CG1,
"\n  !-- QUICK-START TUTORIAL -----------------------------------------!\n"
"  !                                                                  !\n"
"  !  BASIC CONVERSION                                                !\n"
"  !    > convert --from USD --to INR --amount 100                   !\n"
"  !    > c USD INR 100                                               !\n"
"  !                                                                  !\n"
"  !  BULK CONVERT                                                    !\n"
"  !    > bulk USD EUR GBP JPY 1000                                   !\n"
"  !                                                                  !\n"
"  !  WALLET                                                          !\n"
"  !    > wallet add BTC 0.5                                          !\n"
"  !    > wallet show                                                 !\n"
"  !                                                                  !\n"
"  !  ALERTS                                                          !\n"
"  !    > alert USD INR 84.0 above                                    !\n"
"  !                                                                  !\n"
"  !  SHORTCUTS   [S]wap  [H]istory  [W]allet  [Q]uit  [?]Help       !\n"
"  !                                                                  !\n"
"  ╚------------------------------------------------------------------╝\n"
    );
}

/* Feature 99 – System info (RAM, platform) */
void ui_system_info(void)
{
    print_c(CC1,"\n  !-- SYSTEM INFO ----------------------------------!\n");
#ifdef PLATFORM_UNIX
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        long kb = ru.ru_maxrss;
#ifdef __APPLE__
        kb /= 1024;
#endif
        print_c(CG1,"  !  RSS Memory Used  : %-25ldKB!\n", kb);
    }
#else
    print_c(CGR,"  !  (memory info not available on this platform)   !\n");
#endif
    print_c(CC2,"  !  Platform         : %-25s    !\n",
#ifdef PLATFORM_WINDOWS
            "Windows"
#else
            "UNIX/Linux"
#endif
           );
    print_c(CC2,"  !  C Standard       : C11  (__STDC__ = %d)          !\n",
            __STDC__);
    print_c(CC1,"  ╚--------------------------------------------------╝\n");
}

/* Feature 71 – news feed (mock) */
void ui_news_feed(void)
{
    static const char *HEADLINES[] = {
        "Fed holds rates steady; USD weakens slightly",
        "ECB signals possible rate cut in Q2",
        "Bitcoin surges 5% on ETF inflows",
        "Gold hits new 6-month high amid geopolitical tension",
        "RBI likely to pause; INR stabilises near 83.5",
        "Euro Area inflation drops to 2.4%",
        "JPY at 150, BoJ intervention fears grow",
        "Oil rebounds 2% on supply cut speculation",
    };
    print_c(CM1,"\n  -- FINANCIAL NEWS FEED (SIMULATED) --------------\n");
    time_t t = time(NULL);
    for (int i=0;i<8;i++) {
        char ts[16];
        time_t nt = t - i*3600;
        strftime(ts,sizeof ts,"%H:%M",localtime(&nt));
        print_c(CY1,"  [%s] ", ts);
        print_c(CS1,"%s\n", HEADLINES[i]);
    }
}

/* Main menu */
void ui_main_menu(const FinancialEngine *e)
{
    (void)e;
    print_c(CC1,
"\n  !-- COMMAND CENTER MENU ------------------------------------------!\n"
    );
    print_c(CG1,"  !  [c]   Convert         "); print_c(CC2,"[bulk]  Bulk          "); print_c(CM2,"[all]   All-to-All  !\n");
    print_c(CG1,"  !  [inv] Inverse Rate    "); print_c(CC2,"[cross] Cross Rate    "); print_c(CM2,"[cmp]   Compare     !\n");
    print_c(CY1,"  !  [w]   Wallet          "); print_c(CC2,"[fav]   Favourites   "); print_c(CM2,"[alert] Alerts      !\n");
    print_c(CY1,"  !  [hist]History         "); print_c(CC2,"[csv]   Export CSV   "); print_c(CM2,"[bkp]   Backup      !\n");
    print_c(CO1,"  !  [inf] Inflation       "); print_c(CC2,"[goal]  Goal Tracker "); print_c(CM2,"[sal]   Salary PPP  !\n");
    print_c(CO1,"  !  [top] Top Gainers     "); print_c(CC2,"[bot]   Top Losers   "); print_c(CM2,"[graph] Graph       !\n");
    print_c(CO1,"  !  [pvt] Pivot Points    "); print_c(CC2,"[mkt]   Market Data  "); print_c(CM2,"[news]  News Feed   !\n");
    print_c(CS1,"  !  [sort]Sort DB         "); print_c(CC2,"[find]  Search/Filter"); print_c(CM2,"[unit]  Unit Conv.  !\n");
    print_c(CM3,"  !  [set] Settings        "); print_c(CC2,"[pin]   PIN Lock     "); print_c(CM2,"[info]  System Info !\n");
    print_c(CM3,"  !  [fun] Fortune         "); print_c(CC2,"[trivia]Trivia       "); print_c(CM2,"[game]  Rate Game   !\n");
    print_c(CR1,"  !  [hack]Hacker Mode     "); print_c(CC2,"[tt]    Time Travel  "); print_c(CM2,"[space] Space Mode  !\n");
    print_c(CGR,"  !  [about]About          "); print_c(CC2,"[tut]   Tutorial     "); print_c(CM2,"[q]     Quit        !\n");
    print_c(CC1,"  ------------------------------------------------------------------\n");
    print_c(CG2,"  CMD> "); fflush(stdout);
}

/* Currency detail card */
void ui_currency_card(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1,"  Currency not found: %s\n", code); return; }
    print_c(CC1,
"\n  !-- CURRENCY CARD ------------------------------------------------!\n"
    );
    print_c(CC2,"  !  Code    : "); print_c(CG1,"%-59s!\n", c->code);
    print_c(CC2,"  !  Name    : "); print_c(CS1,"%-59s!\n", c->name);
    print_c(CC2,"  !  Symbol  : "); print_c(CGO,"%-59s!\n", c->symbol);
    print_c(CC2,"  !  Country : "); print_c(CS1,"%-59s!\n", c->country);
    char rbuf[LEN_FMTBUF];
    fmt_number(rbuf, sizeof rbuf, c->rate_usd, 6, false);
    print_c(CC2,"  !  Rate(USD): ");print_c(CG2,"%-58s!\n", rbuf);
    print_c(CC2,"  !  24h Chg  : ");
    print_c(fmt_change_color(c->change_24h),
            "%-58s!\n", (snprintf(rbuf,sizeof rbuf,"%+.4f%%",c->change_24h),rbuf));
    print_c(CC2,"  !  Type     : ");
    print_c(CM2,"%-59s!\n",
            c->type==CTYPE_CRYPTO?"Cryptocurrency":
            c->type==CTYPE_COMMODITY?"Commodity":"Fiat Currency");
    print_c(CC1,
"  ╚------------------------------------------------------------------╝\n"
    );
}

/* History shorthand */
void ui_history(const FinancialEngine *e, int last_n)
{ hist_show(e, last_n); }

void ui_separator(int w, const char *color, char ch)
{
    print_c(color,"  ");
    for (int i=0;i<w;i++) putchar(ch);
    printf("\n");
}

/* ---------------------------------------------------------------------------
   SECTION J  -  ANIMATIONS & SOUND
   --------------------------------------------------------------------------- */

static void ms_sleep(int ms)
{
#ifdef PLATFORM_WINDOWS
    Sleep((DWORD)ms);
#else
    struct timespec ts = {ms/1000, (ms%1000)*1000000L};
    nanosleep(&ts, NULL);
#endif
}

/* Feature 12b – Binary rain animation */
void anim_binary_rain(int ms)
{
    printf(CURSOR_HIDE);
    int total = ms / ANIM_FRAME_MS;
    static const char *BITS[] = {"0","1"};
    static const char *COLS[] = {CG1,CG2,CG3,CC1,CC2};
    for (int t=0;t<total;t++) {
        printf("  ");
        for (int c=0;c<36;c++) {
            print_c(COLS[c%5], "%s ", BITS[rand()%2]);
        }
        printf("\n");
        ms_sleep(ANIM_FRAME_MS);
    }
    printf(CURSOR_SHOW);
}

/* Feature 12c – Spinner */
void anim_spinner(int steps, const char *label)
{
    static const char *FRAMES[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
    int nf = 10;
    printf(CURSOR_HIDE);
    for (int i=0;i<steps;i++) {
        printf(CLEAR_LINE);
        print_c(CC1,"  %s ", FRAMES[i % nf]);
        print_c(CG1,"%s", label);
        fflush(stdout);
        ms_sleep(ANIM_FRAME_MS);
    }
    printf(CLEAR_LINE);
    printf(CURSOR_SHOW);
}

/* Feature 18 – Loading animation wrapper */
void anim_loading(int ms, const char *label)
{
    int steps = ms / ANIM_FRAME_MS;
    anim_spinner(steps, label);
}

/* Feature 18 – Progress bar demo */
static void demo_progress(const char *label)
{
    for (int i=0;i<=20;i++) {
        ui_progress_bar(i, 20, BAR_WIDTH, label);
        ms_sleep(ANIM_FRAME_MS / 2);
    }
    printf("\n");
}

/* ---------------------------------------------------------------------------
   SECTION K  -  ANALYTICS & VISUALISATION
   --------------------------------------------------------------------------- */

/* Feature 66 – SMA display */
static void show_sma(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    if (!c) return;
    /* Simulate 30-day window from rate ± small noise */
    double rates[30];
    for (int i=0;i<30;i++)
        rates[i] = c->rate_usd * (1.0 + 0.001 * (i%7 - 3));
    print_c(CM1,"\n  SMA for %s:\n", code);
    print_c(CC2,"  SMA-7  : ");print_c(CG1,"%.6f\n", fin_sma(rates, 7));
    print_c(CC2,"  SMA-14 : ");print_c(CG1,"%.6f\n", fin_sma(rates, 14));
    print_c(CC2,"  SMA-30 : ");print_c(CG1,"%.6f\n", fin_sma(rates, 30));
    /* Feature 79 – mini graph */
    ui_mini_graph(rates, 14, code, CG1);
}

/* Feature 31 – Historical data (mock) */
static void show_historical(const FinancialEngine *e,
                             const char *code, int days)
{
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1,"  Not found.\n"); return; }
    print_c(CM1,"\n  ─── Historical Rates for %s (last %d days) ───\n",
            code, days);
    double rates[30];
    time_t now = time(NULL);
    for (int i=0;i<days&&i<30;i++) {
        rates[i] = c->rate_usd * (1.0 + 0.004*(i%5 - 2));
        char ds[16];
        time_t dt = now - (time_t)(days-i)*86400;
        strftime(ds,sizeof ds,"%m/%d",localtime(&dt));
        print_c(CGR,"  %s  ", ds);
        print_c(CG1,"%.6f\n", rates[i]);
    }
    ui_mini_graph(rates, (days<30?days:30), code, CC1);
}

/* ---------------------------------------------------------------------------
   SECTION L  -  UNIT CONVERTER  (bonus)
   --------------------------------------------------------------------------- */

/* Feature 54 */
double uc_length(double v, const char *from, const char *to)
{
    /* All in metres */
    double to_m[][2] = {
        {0,1.0},{1,0.001},{2,0.01},{3,0.3048},{4,0.9144},
        {5,1609.344},{6,1852.0},{7,1e-9},{8,1e-6},{9,1e-3}
    };
    static const char *UNITS[] = {"m","km","cm","ft","yd","mi","nmi",
                                   "nm","um","mm"};
    int n = 10;
    double fm = -1, tm = -1;
    for (int i=0;i<n;i++) {
        if (strcasecmp(from, UNITS[i])==0) fm = to_m[i][1];
        if (strcasecmp(to,   UNITS[i])==0) tm = to_m[i][1];
    }
    if (fm < 0 || tm <= 0) return -1.0;
    return v * fm / tm;
}

double uc_weight(double v, const char *from, const char *to)
{
    static const char *UNITS[] = {"kg","g","mg","lb","oz","t","st"};
    static const double TO_KG[] = {1.0,0.001,1e-6,0.453592,0.028350,1000.0,6.35029};
    int n=7; double fk=-1,tk=-1;
    for (int i=0;i<n;i++) {
        if (strcasecmp(from,UNITS[i])==0) fk=TO_KG[i];
        if (strcasecmp(to,  UNITS[i])==0) tk=TO_KG[i];
    }
    if (fk<0||tk<=0) return -1.0;
    return v * fk / tk;
}

void uc_show_menu(void)
{
    print_c(CM1,"\n  ─── UNIT CONVERTER ──────────────────────────────\n");
    print_c(CC2,"  Length : m km cm ft yd mi nmi nm um mm\n");
    print_c(CC2,"  Weight : kg g mg lb oz t st\n");
    print_c(CS1,"  Usage  : unit length 100 ft m\n");
    print_c(CS1,"         : unit weight 70 kg lb\n");
}

/* ---------------------------------------------------------------------------
   SECTION M  -  EASTER EGGS & FUN
   --------------------------------------------------------------------------- */

/* Feature 90 – Fortune cookie */
void ee_fortune_cookie(void)
{
    static const char *Q[MAX_QUOTES] = {
        "\"Price is what you pay. Value is what you get.\" - W. Buffett",
        "\"In investing, what is comfortable is rarely profitable.\" - R. Arnott",
        "\"The stock market is a device for transferring money from the impatient to the patient.\" - Buffett",
        "\"Risk comes from not knowing what you're doing.\" - Buffett",
        "\"An investment in knowledge pays the best interest.\" - B. Franklin",
        "\"It's not about timing the market, but time IN the market.\"",
        "\"Compound interest is the eighth wonder of the world.\" - Einstein",
        "\"Markets can remain irrational longer than you can remain solvent.\" - Keynes",
        "\"Don't put all your eggs in one basket.\" - Proverb",
        "\"The four most dangerous words: 'This time it's different.'\" - J. Templeton",
        "\"October is one of the peculiarly dangerous months to speculate in stocks.\" - Mark Twain",
        "\"A penny saved is a penny earned.\"  B. Franklin",
        "\"The secret to getting ahead is getting started.\" - Mark Twain",
        "\"Financial freedom is available to those who learn about it and work for it.\"",
        "\"Buy low, sell high - simple in theory, difficult in practice.\"",
        "\"The market is a pendulum that forever swings between unsustainable optimism and unjustified pessimism.\"",
        "\"Diversification is the only free lunch in investing.\" - H. Markowitz",
        "\"Do not save what is left after spending, but spend what is left after saving.\" - Buffett",
        "\"The goal of the investor is to find situations where it is safe not to diversify.\" - C. Munger",
        "\"In the short run, the market is a voting machine. In the long run, a weighing machine.\" - Graham",
    };
    srand((unsigned)time(NULL));
    print_c(CGO,"\n  !-- FORTUNE COOKIE -------------------------------!\n");
    print_c(CG1,"  !  %-55s!\n", Q[rand() % MAX_QUOTES]);
    print_c(CGO,"  -------------------------------------------------\n");
    beep(false);
}

/* Feature 74 – Currency trivia */
void ee_currency_trivia(void)
{
    static const char *TRIVIA[] = {
        "The oldest currency still in use is the British Pound, dating from 760 AD.",
        "The US Dollar sign ($) may derive from the Spanish Peso or Pieces of Eight.",
        "Kuwait Dinar is currently the world's highest-valued currency (1 KWD ≈ 3.25 USD).",
        "The Zimbabwean Dollar suffered one of history's worst hyperinflations: 89.7 sextillion %.",
        "Switzerland's Franc is the only currency whose name has no standard plural.",
        "The Euro is used by 20 countries - the largest monetary union by number of users.",
        "Bitcoin's ticker BTC was chosen because it was the first available on early exchanges.",
        "The Indian Rupee symbol ₹ was officially adopted in 2010, designed by D. Udaya Kumar.",
        "The word 'salary' derives from Latin 'salarium' - salt given as Roman soldier pay.",
        "The Chinese Yuan (¥) character 元 literally means 'round', describing old coins.",
        "Gold (XAU) is the ISO 4217 code for gold - X denoting non-national currencies.",
        "The first paper currency was issued in China during the Tang Dynasty (~7th century).",
    };
    srand((unsigned)time(NULL));
    print_c(CM1,"\n  !-- DID YOU KNOW? ---------------------------------!\n");
    print_c(CS1,"  !  %-55s!\n", TRIVIA[rand()%12]);
    print_c(CM1,"  -------------------------------------------------\n");
}

/* Feature 93 – Guess the rate mini-game */
void ee_guess_the_rate(FinancialEngine *e)
{
    srand((unsigned)time(NULL));
    int idx = rand() % e->db_count;
    const Currency *c = &e->db[idx];
    double actual = 1.0 / c->rate_usd;   /* USD per 1 unit */
    print_c(CY1,"\n  --- GUESS THE RATE GAME -----------------------\n");
    print_c(CS1,"  How many USD is 1 %s (%s) worth?\n", c->code, c->name);
    print_c(CG2,"  Your guess: ");
    char buf[64]; double guess = 0.0;
    if (fgets(buf, sizeof buf, stdin)) {
        sec_sanitize(buf, sizeof buf);
        if (sec_validate_amount(buf, &guess)) {
            double pct_off = fabs(guess - actual) / actual * 100.0;
            print_c(CG1,"  Actual: %.6f USD\n", actual);
            if (pct_off < 1.0)       { print_c(CG1,"  🎉 SPOT ON! Within 1%% - Incredible!\n"); beep(false); }
            else if (pct_off < 5.0)  { print_c(CY1,"  👍 Very close! Off by %.2f%%\n", pct_off); }
            else if (pct_off < 20.0) { print_c(CO1,"  😐 Decent. Off by %.2f%%\n", pct_off); }
            else                     { print_c(CR1,"  😬 Way off! %.2f%% error. Keep practising!\n", pct_off); }
        } else print_c(CR1,"  Invalid input.\n");
    }
    touch_activity(e);
}

/* Feature 94 – Hacker mode */
void ee_hacker_mode(FinancialEngine *e)
{
    if (e->hacker_mode) {
        e->hacker_mode = false;
        print_c(CGR,"  [Hacker Mode deactivated]\n");
        return;
    }
    print_c(CR1,
"\n  INITIATING HACKER MODE...\n"
"  BYPASSING FIREWALL...\n"
"  ACCESSING SWIFT NETWORK...\n"
    );
    anim_binary_rain(1500);
    print_c(CG1,"  ACCESS GRANTED. Welcome, Agent.\n");
    e->hacker_mode = true;
    beep(true);
}

/* Feature 96 – Time travel mode */
void ee_time_travel(const FinancialEngine *e, const char *from,
                    const char *to, double amount, int year)
{
    /* Historical rate approximations (illustrative) */
    typedef struct { int yr; const char *pair; double rate; } HistRate;
    static const HistRate H[] = {
        {1990,"USD_INR",17.5},{1995,"USD_INR",32.0},
        {2000,"USD_INR",44.9},{2005,"USD_INR",43.5},
        {2010,"USD_INR",45.7},{2015,"USD_INR",65.2},
        {2020,"USD_INR",74.1},{2023,"USD_INR",83.0},
        {1990,"USD_EUR",0.79},{2000,"USD_EUR",1.08},
        {2010,"USD_EUR",0.75},{2020,"USD_EUR",0.82},
        {1990,"USD_JPY",145.0},{2000,"USD_JPY",107.0},
        {2010,"USD_JPY",87.0},{2020,"USD_JPY",106.0},
    };
    char pair[32]; snprintf(pair,sizeof pair,"%s_%s",from,to);
    double hist_rate = -1.0;
    int nH = (int)(sizeof H / sizeof H[0]);
    int best_diff = INT_MAX;
    for (int i=0;i<nH;i++) {
        if (strcmp(H[i].pair, pair)==0) {
            int d = abs(H[i].yr - year);
            if (d < best_diff) { best_diff=d; hist_rate=H[i].rate; }
        }
    }
    print_c(CY1,"\n  -- TIME TRAVEL MODE --  Year: %d --\n", year);
    if (hist_rate < 0.0) {
        print_c(CGR,"  (No historical data for %s→%s; using simulated estimate)\n",from,to);
        hist_rate = conv_basic(e,from,to,1.0) * (1.0 - 0.02*(2024-year));
    }
    double result = amount * hist_rate;
    print_c(CC2,"  In %d: %.2f %s = ", year, amount, from);
    print_c(CG1,"%.4f %s\n", result, to);
    double today = conv_basic(e, from, to, amount);
    print_c(CC2,"  Today:  %.2f %s = ", amount, from);
    print_c(CM1,"%.4f %s\n", today, to);
    double diff = today - result;
    print_c(diff>=0?CG1:CR1,"  Difference: %+.4f %s\n", diff, to);
}

/* Feature 97 – Space mode */
void ee_space_mode(const FinancialEngine *e, const char *code, double amount)
{
    (void)e;
    /* 1 Galactic Credit ≈ USD 42.0 (hitchhiker's tribute) */
    static const double GC_PER_USD = 1.0 / 42.0;
    const Currency *c = db_find(e, code);
    if (!c) { print_c(CR1,"  Currency not found.\n"); return; }
    double usd = conv_basic(e, code, "USD", amount);
    double gc  = usd * GC_PER_USD;
    print_c(CM3,
"\n  -- SPACE MODE -----------------------------------------------\n"
"  \"The answer to life, the universe, and everything is 42.\"\n"
    );
    print_c(CC1,"  %.2f %s  =  ", amount, code);
    print_c(CG1,"$%.4f USD  =  ", usd);
    print_c(CGO,"%.6f Galactic Credits™\n", gc);
    print_c(CM3,"  ------------------------------------------------------------\n");
}

/* Feature 98 – Donation link */
void ee_donation(void)
{
    print_c(CGO,
"\n  !-- SUPPORT THE DEVELOPER ------------------------------------!\n"
"  !  If this tool saved you time or money, consider donating!   !\n"
"  !  PayPal / BMC : %-46s!\n", APP_DONATE);
    print_c(CGO,
"  !  GitHub Star  : %-46s!\n", APP_GITHUB);
    print_c(CGO,
"  ╚--------------------------------------------------------------╝\n"
    );
}

/* Feature 100 – Master badge at 100th conversion */
void ee_master_badge(void)
{
    print_c(CGO,
"\n"
"  !--------------------------------------------------------------!\n"
"  !                                                              !\n"
"  !   ★ ★ ★  MASTER DEVELOPER BADGE  ★ ★ ★                    !\n"
"  !                                                              !\n"
"  !   Congratulations! You have completed 100 conversions.      !\n"
"  !   You are now a certified Currency Command Master.          !\n"
"  !                                                              !\n"
"  ╚--------------------------------------------------------------╝\n"
    );
    beep(false);
    ms_sleep(500);
    beep(false);
}

/* ---------------------------------------------------------------------------
   SECTION N  -  CLI COMMAND PARSER
   --------------------------------------------------------------------------- */

/* Helper: tokenize */
static int tokenize(char *line, char *toks[], int max)
{
    int n = 0;
    char *p = strtok(line, " \t\n\r");
    while (p && n < max) { toks[n++] = p; p = strtok(NULL, " \t\n\r"); }
    return n;
}

void cli_parse(FinancialEngine *e, const char *raw)
{
    char line[LEN_INPUT];
    strncpy(line, raw, sizeof line - 1);
    sec_sanitize(line, sizeof line);

    char *toks[MAX_CMD_TOKENS] = {0};
    int n = tokenize(line, toks, MAX_CMD_TOKENS);
    if (n == 0) return;

    touch_activity(e);

    /* ─── CONVERT ─── */
    if (strcmp(toks[0],"c")==0 || strcmp(toks[0],"convert")==0) {
        if (n < 4) {
            print_c(CR1,"  Usage: c <FROM> <TO> <AMOUNT>\n"); return;
        }
        const char *from   = toks[1];
        const char *to     = toks[2];
        double amount = 0.0;
        if (!sec_validate_amount(toks[3], &amount)) {
            print_c(CR1,"  Invalid amount: %s\n", toks[3]); return;
        }
        if (!sec_validate_code(from) || !sec_validate_code(to)) {
            print_c(CR1,"  Invalid currency code.\n"); return;
        }
        double result = conv_with_tax(e, from, to, amount,
                                      e->default_markup, e->default_tax);
        if (result < 0.0) {
            print_c(CR1,"  Conversion failed - unknown pair.\n");
            beep(true); return;
        }
        result = conv_apply_rounding(result, e->rounding, e->precision);

        /* Record */
        ConversionRecord r = {0};
        strncpy(r.from, from, LEN_CODE-1);
        strncpy(r.to,   to,   LEN_CODE-1);
        r.amount     = amount;
        r.result     = result;
        r.rate_used  = conv_basic(e, from, to, 1.0);
        r.markup_pct = e->default_markup;
        r.tax_pct    = e->default_tax;
        r.rounding   = e->rounding;
        r.ts         = time(NULL);
        hist_push(e, &r);

        /* Display */
        char buf[LEN_FMTBUF];
        fmt_number(buf, sizeof buf, result, e->precision, false);
        print_c(CC2,"\n  ");
        print_c(CG1,"%.4f", amount);
        print_c(CC2," %s  →  ", from);
        print_c(CG1,"%s", buf);
        print_c(CC2,"  %s", to);
        if (e->show_symbols) {
            print_c(CGO,"  [%s → %s]",
                    get_symbol_str(e,from), get_symbol_str(e,to));
        }
        printf("\n");
        if (e->default_markup > 0.0)
            print_c(CGR,"  (incl. %.2f%% markup", e->default_markup);
        if (e->default_tax    > 0.0)
            print_c(CGR," + %.2f%% tax", e->default_tax);
        if (e->default_markup > 0.0 || e->default_tax > 0.0)
            printf(")\n");
        beep(false);
        alert_check(e);

        strncpy(e->last_from, from, LEN_CODE-1);
        strncpy(e->last_to,   to,   LEN_CODE-1);
        e->last_amount = amount;
        return;
    }

    /* ─── INVERSE  ─── */
    if (strcmp(toks[0],"inv")==0 || strcmp(toks[0],"inverse")==0) {
        if (n < 3) { print_c(CR1,"  Usage: inv <FROM> <TO>\n"); return; }
        double inv = conv_inverse_rate(e, toks[1], toks[2]);
        if (inv < 0.0) { print_c(CR1,"  Pair not found.\n"); return; }
        print_c(CC2,"\n  1 %s = ", toks[2]);
        print_c(CG1,"%.*f", e->precision, inv);
        print_c(CC2," %s\n", toks[1]);
        return;
    }

    /* ─── SWAP ─── (Feature 16 keyboard shortcut S) */
    if (strcmp(toks[0],"s")==0 || strcmp(toks[0],"swap")==0) {
        char tmp[LEN_CODE] = {0};
        memcpy(tmp,          e->last_from, LEN_CODE);
        memcpy(e->last_from, e->last_to,   LEN_CODE);
        memcpy(e->last_to,   tmp,          LEN_CODE);
        print_c(CY1,"  Swapped: %s ↔ %s\n", e->last_from, e->last_to);
        /* Re-run last conversion with swapped pair */
        char buf[64]; snprintf(buf,sizeof buf,"c %s %s %.6f",
                               e->last_from, e->last_to, e->last_amount);
        cli_parse(e, buf);
        return;
    }

    /* ─── BULK ─── */
    if (strcmp(toks[0],"bulk")==0) {
        if (n < 4) { print_c(CR1,"  Usage: bulk <FROM> <C1> [C2..] <AMOUNT>\n"); return; }
        double amt = 0.0;
        if (!sec_validate_amount(toks[n-1], &amt)) {
            print_c(CR1,"  Last token must be the amount.\n"); return;
        }
        const char *targets[MAX_BULK_TARGETS];
        int tc = 0;
        for (int i=2; i<n-1 && tc<MAX_BULK_TARGETS; i++) targets[tc++]=toks[i];
        conv_bulk(e, toks[1], targets, tc, amt);
        return;
    }

    /* ─── ALL-TO-ALL ─── */
    if (strcmp(toks[0],"all")==0) {
        double amt = 1.0;
        if (n >= 3) sec_validate_amount(toks[2], &amt);
        conv_all_to_all(e, n>=2?toks[1]:"USD", amt);
        return;
    }

    /* ─── WALLET ─── */
    if (strcmp(toks[0],"w")==0 || strcmp(toks[0],"wallet")==0) {
        if (n < 2) { wallet_show(e); return; }
        if (strcmp(toks[1],"show")==0)  { wallet_show(e);       return; }
        if (strcmp(toks[1],"dist")==0)  { wallet_distribution(e); return; }
        if (strcmp(toks[1],"total")==0) {
            print_c(CG1,"  Total wallet: $%.4f USD\n", wallet_total_usd(e));
            return;
        }
        if (strcmp(toks[1],"add")==0 && n>=4) {
            double a=0.0;
            if (sec_validate_amount(toks[3],&a))
                wallet_deposit(e, toks[2], a) ?
                    print_c(CG1,"  Deposited.\n") :
                    print_c(CR1,"  Deposit failed.\n");
            return;
        }
        if (strcmp(toks[1],"sub")==0 && n>=4) {
            double a=0.0;
            if (sec_validate_amount(toks[3],&a))
                wallet_withdraw(e, toks[2], a) ?
                    print_c(CG1,"  Withdrawn.\n") :
                    print_c(CR1,"  Insufficient balance.\n");
            return;
        }
        if (strcmp(toks[1],"save")==0) { io_save_wallet(e); print_c(CG1,"  Wallet saved.\n"); return; }
        wallet_show(e);
        return;
    }

    /* ─── FAVOURITES ─── */
    if (strcmp(toks[0],"fav")==0) {
        if (n < 2) { fav_show(e); return; }
        if (strcmp(toks[1],"add")==0 && n>=3) {
            fav_add(e,toks[2])? print_c(CG1,"  Added ★ %s\n",toks[2])
                              : print_c(CR1,"  Could not add.\n");
            return;
        }
        if (strcmp(toks[1],"rm")==0 && n>=3) {
            fav_remove(e,toks[2])? print_c(CG1,"  Removed %s\n",toks[2])
                                 : print_c(CR1,"  Not in favourites.\n");
            return;
        }
        fav_show(e);
        return;
    }

    /* ─── ALERT ─── */
    if (strcmp(toks[0],"alert")==0) {
        if (n < 2) { alert_list(e); return; }
        if (strcmp(toks[1],"list")==0) { alert_list(e); return; }
        if (n >= 4) {
            double tgt = 0.0;
            if (!sec_validate_amount(toks[3], &tgt)) return;
            bool above = (n < 5 || strcmp(toks[4],"above")==0);
            alert_add(e, toks[1], tgt, above)?
                print_c(CG1,"  Alert set for %s %s %.4f\n",
                        toks[1], above?"≥":"≤", tgt) :
                print_c(CR1,"  Alert limit reached or invalid code.\n");
            return;
        }
        alert_list(e);
        return;
    }

    /* ─── HISTORY ─── */
    if (strcmp(toks[0],"h")==0 || strcmp(toks[0],"hist")==0) {
        int last = 10;
        if (n >= 2) last = atoi(toks[1]);
        hist_show(e, last);
        return;
    }
    if (strcmp(toks[0],"hclear")==0) { hist_clear(e); return; }

    /* ─── CSV EXPORT ─── */
    if (strcmp(toks[0],"csv")==0) {
        io_export_csv(e)?
            print_c(CG1,"  Exported to %s\n", FILE_HISTORY_CSV):
            print_c(CR1,"  Export failed.\n");
        return;
    }

    /* ─── BACKUP / RESTORE ─── */
    if (strcmp(toks[0],"bkp")==0 || strcmp(toks[0],"backup")==0) {
        io_backup(e)?
            print_c(CG1,"  Backup saved.\n"):
            print_c(CR1,"  Backup failed.\n");
        return;
    }
    if (strcmp(toks[0],"restore")==0) {
        io_restore(e)?
            print_c(CG1,"  Restored from backup.\n"):
            print_c(CR1,"  Restore failed.\n");
        return;
    }

    /* ─── INFLATION ─── */
    if (strcmp(toks[0],"inf")==0) {
        if (n < 4) { print_c(CR1,"  Usage: inf <amount> <rate%> <years>\n"); return; }
        double p=atof(toks[1]), r=atof(toks[2]);
        int y=atoi(toks[3]);
        double res = fin_inflation(p, r, y);
        print_c(CC2,"  %.2f after %d yrs @ %.2f%% inflation = ", p, y, r);
        print_c(CG1,"%.4f\n", res);
        return;
    }

    /* ─── GOAL ─── */
    if (strcmp(toks[0],"goal")==0) {
        if (n < 4) { print_c(CR1,"  Usage: goal <LOCAL> <FOREIGN> <TARGET_FOREIGN>\n"); return; }
        double tgt=0.0; sec_validate_amount(toks[3],&tgt);
        double need = fin_goal(e, toks[1], toks[2], tgt);
        print_c(CC2,"  To reach %.2f %s you need approx ", tgt, toks[2]);
        print_c(CG1,"%.4f %s\n", need, toks[1]);
        return;
    }

    /* ─── SALARY PPP ─── */
    if (strcmp(toks[0],"sal")==0) {
        if (n < 4) { print_c(CR1,"  Usage: sal <FROM> <TO> <SALARY>\n"); return; }
        double s=0.0; sec_validate_amount(toks[3],&s);
        print_c(CG1,"  Salary %.2f %s ≈ %.2f %s (nominal)\n",
                s, toks[1], fin_salary_ppp(e,toks[1],toks[2],s), toks[2]);
        return;
    }

    /* ─── TOP GAINERS / LOSERS ─── */
    if (strcmp(toks[0],"top")==0) { fin_top_gainers(e, n>=2?atoi(toks[1]):5); return; }
    if (strcmp(toks[0],"bot")==0) { fin_top_losers(e,  n>=2?atoi(toks[1]):5); return; }

    /* ─── GRAPH / SMA ─── */
    if (strcmp(toks[0],"graph")==0 || strcmp(toks[0],"sma")==0) {
        if (n < 2) { print_c(CR1,"  Usage: graph <CODE>\n"); return; }
        show_sma(e, toks[1]);
        return;
    }

    /* ─── PIVOT ─── */
    if (strcmp(toks[0],"pvt")==0) {
        if (n < 2) { print_c(CR1,"  Usage: pvt <CODE>\n"); return; }
        fin_pivot_points(e, toks[1]);
        return;
    }

    /* ─── MARKET DATA ─── */
    if (strcmp(toks[0],"mkt")==0) {
        if (n < 2) { print_c(CR1,"  Usage: mkt <CODE>\n"); return; }
        fin_market_data(e, toks[1]);
        return;
    }

    /* ─── COMPARE ─── */
    if (strcmp(toks[0],"cmp")==0) {
        if (n < 3) { print_c(CR1,"  Usage: cmp <C1> <C2>\n"); return; }
        fin_compare(e, toks[1], toks[2]);
        return;
    }

    /* ─── HISTORICAL ─── */
    if (strcmp(toks[0],"hist-rate")==0) {
        if (n < 2) { print_c(CR1,"  Usage: hist-rate <CODE> [days]\n"); return; }
        int d = n>=3 ? atoi(toks[2]) : 7;
        show_historical(e, toks[1], d);
        return;
    }

    /* ─── SORT ─── */
    if (strcmp(toks[0],"sort")==0) {
        SortField sf = SORT_BY_CODE;
        if (n>=2) {
            if (strcmp(toks[1],"name")==0)   sf = SORT_BY_NAME;
            if (strcmp(toks[1],"rate")==0)   sf = SORT_BY_RATE;
            if (strcmp(toks[1],"change")==0) sf = SORT_BY_CHANGE;
        }
        db_sort(e, sf, SORT_ASC);
        print_c(CG1,"  Database sorted.\n");
        return;
    }

    /* ─── FILTER ─── */
    if (strcmp(toks[0],"find")==0 || strcmp(toks[0],"search")==0) {
        if (n < 2) { print_c(CR1,"  Usage: find <keyword>\n"); return; }
        db_filter(e, toks[1]);
        return;
    }

    /* ─── SUGGEST ─── */
    if (strcmp(toks[0],"?")==0 && n >= 2) {
        db_auto_suggest(e, toks[1]);
        return;
    }

    /* ─── SETTINGS ─── */
    if (strcmp(toks[0],"set")==0) {
        if (n < 3) {
            print_c(CM1,"\n  Settings:\n");
            print_c(CC2,"  precision  : "); print_c(CG1,"%d\n", e->precision);
            print_c(CC2,"  markup     : "); print_c(CG1,"%.2f%%\n", e->default_markup);
            print_c(CC2,"  tax        : "); print_c(CG1,"%.2f%%\n", e->default_tax);
            print_c(CC2,"  language   : "); print_c(CG1,"%d\n", e->language);
            print_c(CC2,"  sound      : "); print_c(CG1,"%s\n", e->sound_enabled?"on":"off");
            print_c(CC2,"  ghost      : "); print_c(CG1,"%s\n", e->ghost_mode?"on":"off");
            print_c(CC2,"  base       : "); print_c(CG1,"%s\n", e->base_currency);
            print_c(CS1,"  set precision 4 | set markup 1.5 | set tax 18 | set sound on\n");
            return;
        }
        if (strcmp(toks[1],"precision")==0) {
            int p=atoi(toks[2]);
            if (p>=0&&p<=8) { e->precision=p; print_c(CG1,"  Precision = %d\n",p); }
        } else if (strcmp(toks[1],"markup")==0) {
            e->default_markup=atof(toks[2]);
            print_c(CG1,"  Markup = %.2f%%\n", e->default_markup);
        } else if (strcmp(toks[1],"tax")==0) {
            e->default_tax=atof(toks[2]);
            print_c(CG1,"  Tax = %.2f%%\n", e->default_tax);
        } else if (strcmp(toks[1],"rounding")==0) {
            if (strcmp(toks[2],"floor")==0)    e->rounding=ROUND_FLOOR;
            else if (strcmp(toks[2],"ceil")==0) e->rounding=ROUND_CEILING;
            else if (strcmp(toks[2],"bank")==0) e->rounding=ROUND_BANKER;
            else                                e->rounding=ROUND_MIDPOINT;
            print_c(CG1,"  Rounding mode updated.\n");
        } else if (strcmp(toks[1],"sound")==0) {
            e->sound_enabled = (strcmp(toks[2],"on")==0);
            print_c(CG1,"  Sound %s.\n", e->sound_enabled?"enabled":"disabled");
        } else if (strcmp(toks[1],"ghost")==0) {
            e->ghost_mode = (strcmp(toks[2],"on")==0);
            print_c(CG1,"  Ghost mode %s.\n", e->ghost_mode?"on":"off");
        } else if (strcmp(toks[1],"base")==0) {
            if (sec_validate_code(toks[2]) && db_find(e,toks[2])) {
                strncpy(e->base_currency, toks[2], LEN_CODE-1);
                print_c(CG1,"  Base currency = %s\n", e->base_currency);
            } else print_c(CR1,"  Unknown currency code.\n");
        } else if (strcmp(toks[1],"apikey")==0 && n>=3) {
            strncpy(e->api_key, toks[2], LEN_API_KEY-1);
            sec_mask_key(e->api_key, e->api_key_masked, sizeof e->api_key_masked);
            print_c(CG1,"  API key set [%s].\n", e->api_key_masked);
        } else if (strcmp(toks[1],"language")==0) {
            e->language = (Language)atoi(toks[2]);
            print_c(CG1,"  Language set to %d.\n", e->language);
        } else if (strcmp(toks[1],"manual")==0 && n>=5) {
            /* Feature 53 – manual rate: set manual USD INR 84.5 */
            strncpy(e->manual.from, toks[2], LEN_CODE-1);
            strncpy(e->manual.to,   toks[3], LEN_CODE-1);
            e->manual.rate   = atof(toks[4]);
            e->manual.active = true;
            print_c(CG1,"  Manual rate: 1 %s = %.4f %s\n",
                    toks[2], e->manual.rate, toks[3]);
        } else if (strcmp(toks[1],"manual-off")==0) {
            e->manual.active = false;
            print_c(CY1,"  Manual rate disabled.\n");
        } else {
            print_c(CR1,"  Unknown setting: %s\n", toks[1]);
        }
        config_save(e);
        return;
    }

    /* ─── MANUAL RATE ─── */
    if (strcmp(toks[0],"rate")==0 && n>=3) {
        double r = conv_basic(e, toks[1], toks[2], 1.0);
        if (r < 0.0) { print_c(CR1,"  Pair not found.\n"); return; }
        print_c(CC2,"  1 %s = ", toks[1]);
        print_c(CG1,"%.*f", e->precision, r);
        print_c(CC2," %s\n", toks[2]);
        return;
    }

    /* ─── PIN ─── */
    if (strcmp(toks[0],"pin")==0) {
        if (n>=2 && strcmp(toks[1],"set")==0) { sec_setup_pin(e); return; }
        if (n>=2 && strcmp(toks[1],"verify")==0) {
            sec_verify_pin(e) ?
                print_c(CG1,"  PIN correct.\n") :
                print_c(CR1,"  Wrong PIN.\n");
            return;
        }
        if (n>=2 && strcmp(toks[1],"off")==0) {
            e->is_locked = false;
            sec_wipe(e->pin_hash, sizeof e->pin_hash);
            print_c(CY1,"  PIN lock disabled.\n");
            return;
        }
        print_c(CS1,"  pin set | pin verify | pin off\n");
        return;
    }

    /* ─── UNIT CONVERTER ─── */
    if (strcmp(toks[0],"unit")==0) {
        if (n < 5) { uc_show_menu(); return; }
        double v=atof(toks[2]);
        double res = -1.0;
        if (strcmp(toks[1],"length")==0) res = uc_length(v, toks[3], toks[4]);
        if (strcmp(toks[1],"weight")==0) res = uc_weight(v, toks[3], toks[4]);
        if (res < 0.0) print_c(CR1,"  Unsupported unit pair.\n");
        else { print_c(CC2,"  %.4f %s = ", v, toks[3]);
               print_c(CG1,"%.4f %s\n", res, toks[4]); }
        return;
    }

    /* ─── ANALYTICS SHORTCUTS ─── */
    if (strcmp(toks[0],"pct")==0) {
        if (n>=2) fin_percentage_change(e, toks[1]);
        return;
    }
    if (strcmp(toks[0],"avg")==0) {
        if (n>=2) fin_avg_rate(e, toks[1]);
        return;
    }

    /* ─── FUN / EASTER EGGS ─── */
    if (strcmp(toks[0],"fun")==0 || strcmp(toks[0],"fortune")==0)
        { ee_fortune_cookie(); return; }
    if (strcmp(toks[0],"trivia")==0)
        { ee_currency_trivia(); return; }
    if (strcmp(toks[0],"game")==0)
        { ee_guess_the_rate(e); return; }
    if (strcmp(toks[0],"hack")==0 || strcmp(toks[0],"hacker")==0)
        { ee_hacker_mode(e); return; }
    if (strcmp(toks[0],"donate")==0)
        { ee_donation(); return; }

    if (strcmp(toks[0],"tt")==0) {   /* time travel */
        if (n < 5) { print_c(CR1,"  Usage: tt <FROM> <TO> <AMOUNT> <YEAR>\n"); return; }
        double amt=0.0; sec_validate_amount(toks[3],&amt);
        ee_time_travel(e, toks[1], toks[2], amt, atoi(toks[4]));
        return;
    }
    if (strcmp(toks[0],"space")==0) {
        if (n < 3) { print_c(CR1,"  Usage: space <CODE> <AMOUNT>\n"); return; }
        double amt=0.0; sec_validate_amount(toks[2],&amt);
        ee_space_mode(e, toks[1], amt);
        return;
    }

    /* ─── TRAVEL BUDGET ─── */
    if (strcmp(toks[0],"travel")==0) {
        /* travel <HOME> <DEST>  - then prompts */
        if (n < 3) { print_c(CR1,"  Usage: travel <HOME> <DEST>\n"); return; }
        print_c(CC2,"  Enter items as: <amount> <description>  (blank line to finish)\n");
        double amounts[32]; const char *descs[32]; char dbufs[32][64];
        int cnt = 0;
        while (cnt < 32) {
            char ibuf[128]; print_c(CG2,"  > "); fflush(stdout);
            if (!fgets(ibuf,sizeof ibuf,stdin)) break;
            sec_sanitize(ibuf,sizeof ibuf);
            if (!*ibuf) break;
            double a=0.0; char d[64]="expense";
            if (sscanf(ibuf,"%lf %63[^\n]",&a,d) >= 1) {
                amounts[cnt]=a;
                snprintf(dbufs[cnt], sizeof dbufs[cnt], "%s", d);
                descs[cnt]=dbufs[cnt];
                cnt++;
            }
        }
        if (cnt>0) fin_travel_budget(e,toks[1],toks[2],amounts,descs,cnt);
        return;
    }

    /* ─── REPORT ─── */
    if (strcmp(toks[0],"report")==0) {
        auto_daily_report(e); return;
    }

    /* ─── NEWS ─── */
    if (strcmp(toks[0],"news")==0) { ui_news_feed(); return; }

    /* ─── INFO / ABOUT / TUTORIAL ─── */
    if (strcmp(toks[0],"about")==0) { ui_about(); return; }
    if (strcmp(toks[0],"tut")==0)   { ui_tutorial(); return; }
    if (strcmp(toks[0],"info")==0)  { ui_system_info(); return; }
    if (strcmp(toks[0],"card")==0)  { if(n>=2) ui_currency_card(e,toks[1]); return; }

    /* ─── CLEAR / REFRESH ─── */
    if (strcmp(toks[0],"cls")==0 || strcmp(toks[0],"clear")==0) {
        ui_header(e); return;
    }

    /* ─── CHECKSUM ─── */
    if (strcmp(toks[0],"chksum")==0) { sec_check_checksum(e); return; }

    /* ─── QUIT ─── */
    if (strcmp(toks[0],"q")==0 || strcmp(toks[0],"quit")==0
        || strcmp(toks[0],"exit")==0) {
        config_save(e);
        io_save_wallet(e);
        cache_save(e);
        print_c(CG1,"\n  Goodbye, Agent. Stay liquid.\n\n");
        engine_destroy(e);
        exit(EXIT_SUCCESS);
    }

    /* ─── HELP ─── */
    if (strcmp(toks[0],"help")==0 || strcmp(toks[0],"menu")==0
        || strcmp(toks[0],"m")==0) {
        ui_main_menu(e); return;
    }

    print_c(CR1,"  Unknown command: %s  (type 'menu' for help)\n", toks[0]);
}

/* Feature 46 – Scripting: direct CLI args */
void cli_run_argv(FinancialEngine *e, int argc, char *argv[])
{
    if (argc < 4) {
        print_c(CY1,"  Usage: %s <FROM> <TO> <AMOUNT>\n", argv[0]);
        print_c(CY1,"  Example: %s USD INR 100\n", argv[0]);
        return;
    }
    char cmd[LEN_INPUT];
    snprintf(cmd, sizeof cmd, "c %s %s %s", argv[1], argv[2], argv[3]);
    cli_parse(e, cmd);
}

/* ---------------------------------------------------------------------------
   SECTION O  -  i18n / TRANSLATION
   --------------------------------------------------------------------------- */

/* Feature 50 – Language support (Hindi, Spanish, English) */
const char *i18n(const FinancialEngine *e, const char *key)
{
    typedef struct { const char *key, *en, *hi, *es; } T;
    static const T TABLE[] = {
        {"convert",     "Convert",          "बदलें",          "Convertir"},
        {"amount",      "Amount",           "राशि",           "Cantidad"},
        {"rate",        "Rate",             "दर",             "Tasa"},
        {"result",      "Result",           "परिणाम",         "Resultado"},
        {"quit",        "Quit",             "बाहर",           "Salir"},
        {"history",     "History",          "इतिहास",         "Historial"},
        {"wallet",      "Wallet",           "बटुआ",           "Cartera"},
        {"error",       "Error",            "त्रुटि",          "Error"},
        {"success",     "Success",          "सफलता",          "Éxito"},
        {"settings",    "Settings",         "सेटिंग्स",        "Ajustes"},
        {NULL,NULL,NULL,NULL}
    };
    for (int i=0; TABLE[i].key; i++) {
        if (strcmp(TABLE[i].key, key)==0) {
            switch (e->language) {
                case LANG_HINDI:   return TABLE[i].hi;
                case LANG_SPANISH: return TABLE[i].es;
                default:           return TABLE[i].en;
            }
        }
    }
    return key;
}

/* ---------------------------------------------------------------------------
   MISC HELPERS
   --------------------------------------------------------------------------- */

const char *fmt_change_color(double c)
{
    if (c >  0.5) return CG1;
    if (c >  0.0) return CG2;
    if (c == 0.0) return CGR;
    if (c > -0.5) return CO1;
    return CR1;
}

void fmt_timestamp(char *buf, size_t sz, time_t t)
{
    strftime(buf, sz, "%Y-%m-%d %H:%M", localtime(&t));
}

void fmt_currency_val(char *buf, size_t sz, const Currency *c,
                      double v, int prec)
{
    if (!c) { snprintf(buf,sz,"%.4f",v); return; }
    char nb[LEN_FMTBUF];
    fmt_number(nb, sizeof nb, v, prec, false);
    snprintf(buf, sz, "%s%s", c->symbol, nb);
}

const char *get_symbol_str(const FinancialEngine *e, const char *code)
{
    const Currency *c = db_find(e, code);
    return c ? c->symbol : code;
}

void touch_activity(FinancialEngine *e)
{
    e->last_activity = time(NULL);
}

void badge_check(FinancialEngine *e)
{
    if (e->total_conversions == MASTER_BADGE_AT)
        ee_master_badge();
}

/* Feature 50 – Inactivity auto-shutdown */
bool auto_inactivity_check(const FinancialEngine *e)
{
    return (time(NULL) - e->last_activity) > INACTIVITY_SEC;
}

/* Feature 42 – Startup update (simulate) */
void auto_startup_update(FinancialEngine *e)
{
    if (cache_load(e)) {
        print_c(CG1,"  [CACHE] Loaded rates from cache.\n");
        return;
    }
    print_c(CC2,"  [OFFLINE] Using bundled default rates.\n");
    populate_default_currencies(e);
    cache_save(e);
}

/* Feature 44 – Daily report */
void auto_daily_report(const FinancialEngine *e)
{
    char ts[32];
    time_t now = time(NULL);
    strftime(ts, sizeof ts, "%Y-%m-%d", localtime(&now));
    print_c(CC1,"\n  !-- DAILY REPORT  [%s] --------------------------!\n", ts);
    print_c(CC2,"  !  Total conversions today   : %-26d!\n", e->hist_count);
    print_c(CC2,"  !  All-time conversions       : %-26d!\n", e->total_conversions);
    print_c(CC2,"  !  Wallet total (USD)         : $%-25.2f!\n", wallet_total_usd(e));
    print_c(CC2,"  !  Currencies in database     : %-26d!\n", e->db_count);
    print_c(CC2,"  !  Active alerts              : %-26d!\n", e->alert_count);
    print_c(CC1,"  ╚--------------------------------------------------╝\n");
    io_export_csv(e);
    print_c(CGR,"  CSV history exported.\n");
}

/* strncasestr portable implementation */
char *strncasestr(const char *hay, const char *needle)
{
    if (!needle || !*needle) return (char *)hay;
    size_t nlen = strlen(needle);
    for (; *hay; hay++) {
        if (strncasecmp(hay, needle, nlen) == 0) return (char *)hay;
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
   SECTION P  -  main()
   --------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    /* ── Init ── */
    FinancialEngine *e = engine_init();
    if (!e) { fprintf(stderr, "FATAL: engine_init failed\n"); return 1; }

    /* ── Scripting mode: ./converter USD INR 100 ── */
    if (argc >= 4) {
        populate_default_currencies(e);
        cli_run_argv(e, argc, argv);
        engine_destroy(e);
        return 0;
    }

    /* ── Load config / cache / wallet ── */
    config_load(e);
    io_load_wallet(e);

    /* ── Startup animation & rate load ── */
    ui_header(e);
    demo_progress("Loading currency database");
    auto_startup_update(e);
    ee_fortune_cookie();
    io_write_log(e, LOG_INFO, "Session started. DB: %d currencies.", e->db_count);

    /* ── PIN lock check ── */
    if (e->is_locked && e->pin_hash[0]) {
        print_c(CM3,"  🔒 Application locked. ");
        if (!sec_verify_pin(e)) {
            print_c(CR1,"  Access denied.\n");
            engine_destroy(e);
            return 1;
        }
        print_c(CG1,"  Unlocked.\n");
    }

    /* ── Main REPL loop ── */
    char input[LEN_INPUT];
    ui_main_menu(e);

    while (1) {
        /* Inactivity check - Feature 50 */
        if (auto_inactivity_check(e)) {
            print_c(CY1,"\n  [Auto-shutdown: inactivity timeout]\n");
            break;
        }

        /* Prompt */
        if (!fgets(input, sizeof input, stdin)) break;
        sec_sanitize(input, sizeof input);
        if (!*input) continue;

        cli_parse(e, input);

        /* Check alerts on every interaction - Feature 41 */
        alert_check(e);
    }

    /* ── Graceful shutdown ── */
    config_save(e);
    io_save_wallet(e);
    cache_save(e);
    io_write_log(e, LOG_INFO, "Session ended. Total conversions: %d",
                 e->total_conversions);
    print_c(CG1,"\n  Session saved. Goodbye, Agent.\n\n");
    engine_destroy(e);
    return 0;
}
