#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <limits>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <functional>
#include <optional>
#include <variant>
#include <random>
#include <thread>
#include <ctime>
#include <cassert>
#include <climits>
#include <typeinfo>
#include <cstring>

// ─── Platform Detection ─────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
    #define CLEAR_SCREEN "cls"
    #define ARCH_BITS (sizeof(void*) * 8)
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #define CLEAR_SCREEN "clear"
    #define ARCH_BITS (sizeof(void*) * 8)
    #include <sys/sysinfo.h>
#elif defined(__APPLE__)
    #define PLATFORM_MAC
    #define CLEAR_SCREEN "clear"
    #define ARCH_BITS (sizeof(void*) * 8)
#else
    #define PLATFORM_UNKNOWN
    #define CLEAR_SCREEN "clear"
    #define ARCH_BITS 64
#endif

// ─── ANSI Color Codes ────────────────────────────────────────
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";
    const std::string BLINK   = "\033[5m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
    const std::string BG_RED  = "\033[41m";
    const std::string BG_BLUE = "\033[44m";
    const std::string DIM     = "\033[2m";
}

// ─── Constants ───────────────────────────────────────────────
namespace Constants {
    constexpr long double ABSOLUTE_ZERO_C  = -273.15L;
    constexpr long double ABSOLUTE_ZERO_K  =    0.0L;
    constexpr long double WATER_BOIL_C     =  100.0L;
    constexpr long double WATER_FREEZE_C   =    0.0L;
    constexpr long double ROOM_TEMP_C      =   22.0L;
    constexpr long double BODY_TEMP_C      =   37.0L;
    constexpr long double FEVER_THRESHOLD  =   38.0L;
    constexpr long double DRY_ICE_SUBLIM_C = -78.5L;
    constexpr long double VERSION_MAJOR    =    2.0L;
    constexpr long double VERSION_MINOR    =    0.0L;
    constexpr int         MAX_HISTORY      =  1000;
    constexpr double      SPECIFIC_HEAT_WATER = 4186.0; // J/(kg·K)
}

// ═══════════════════════════════════════════════════════════════
// SECTION 1: TEMPLATE CONVERTER ENGINE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Generic Converter<T> — handles float, double, long double.
 *        All conversions go through Kelvin to avoid chained precision loss.
 *        Feature coverage: #1-#9, #11-#12, #44
 */
template<typename T>
class Converter {
    static_assert(std::is_floating_point<T>::value,
                  "Converter<T> requires a floating-point type.");
public:
    // ── Core conversion formulas ────────────────────────────
    static T celsiusToKelvin   (T c) { return c + static_cast<T>(273.15); }
    static T kelvinToCelsius   (T k) { return k - static_cast<T>(273.15); }
    static T celsiusToFahrenheit(T c){ return c * static_cast<T>(9.0/5.0) + static_cast<T>(32); }
    static T fahrenheitToCelsius(T f){ return (f - static_cast<T>(32)) * static_cast<T>(5.0/9.0); }
    static T fahrenheitToKelvin(T f) { return celsiusToKelvin(fahrenheitToCelsius(f)); }
    static T kelvinToFahrenheit(T k) { return celsiusToFahrenheit(kelvinToCelsius(k)); }
    // Rankine scale (Feature #4)
    static T celsiusToRankine  (T c) { return celsiusToKelvin(c) * static_cast<T>(9.0/5.0); }
    static T rankineToCelsius  (T r) { return kelvinToCelsius(r * static_cast<T>(5.0/9.0)); }
    // Réaumur scale (Feature #5)
    static T celsiusToReaumur  (T c) { return c * static_cast<T>(4.0/5.0); }
    static T reaumurToCelsius  (T r) { return r * static_cast<T>(5.0/4.0); }

    /**
     * @brief Validates temperature against absolute zero.
     * @throws std::domain_error if below absolute zero. (Feature #9)
     */
    static void validateKelvin(T kelvin) {
        if (kelvin < Constants::ABSOLUTE_ZERO_K - static_cast<T>(1e-9)) {
            throw std::domain_error(
                "Temperature below absolute zero (-273.15°C / 0 K). "
                "Physically impossible value.");
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 2: TEMPERATURE CLASS (Core Engine)
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Core Temperature class. Stores value internally as Kelvin (long double)
 *        for maximum precision. Converts on-the-fly for output.
 *        Features: #1-#10, #20, #55
 */
class Temperature {
private:
    long double kelvin_;   ///< Internal storage in Kelvin
    int         precision_; ///< Decimal places for display (Feature #10)

public:
    enum class Scale { CELSIUS, FAHRENHEIT, KELVIN, RANKINE, REAUMUR };

    /**
     * @brief Constructor — accepts value in any scale, stores as Kelvin.
     * @throws std::domain_error on below-absolute-zero value.
     */
    explicit Temperature(long double value, Scale scale = Scale::CELSIUS, int prec = 4)
        : precision_(prec)
    {
        switch (scale) {
            case Scale::CELSIUS:
                kelvin_ = Converter<long double>::celsiusToKelvin(value); break;
            case Scale::FAHRENHEIT:
                kelvin_ = Converter<long double>::fahrenheitToKelvin(value); break;
            case Scale::KELVIN:
                kelvin_ = value; break;
            case Scale::RANKINE:
                kelvin_ = value * (5.0L / 9.0L); break;
            case Scale::REAUMUR:
                kelvin_ = Converter<long double>::celsiusToKelvin(
                              Converter<long double>::reaumurToCelsius(value)); break;
        }
        Converter<long double>::validateKelvin(kelvin_);
    }

    // ── Getters for each scale ───────────────────────────────
    long double asCelsius()    const { return Converter<long double>::kelvinToCelsius(kelvin_); }
    long double asFahrenheit() const { return Converter<long double>::kelvinToFahrenheit(kelvin_); }
    long double asKelvin()     const { return kelvin_; }
    long double asRankine()    const { return kelvin_ * (9.0L / 5.0L); }
    long double asReaumur()    const { return Converter<long double>::celsiusToReaumur(asCelsius()); }
    int         getPrecision() const { return precision_; }
    void        setPrecision(int p)  { precision_ = (p >= 0 && p <= 15) ? p : 4; }

    /**
     * @brief Formats a value with the configured precision and unit symbol.
     *        Feature #20 (auto-suffix), Feature #10 (precision control).
     */
    std::string format(Scale scale) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision_);
        switch (scale) {
            case Scale::CELSIUS:
                oss << asCelsius()    << " \u00B0C";  break;
            case Scale::FAHRENHEIT:
                oss << asFahrenheit() << " \u00B0F";  break;
            case Scale::KELVIN:
                oss << asKelvin()     << " K";        break;
            case Scale::RANKINE:
                oss << asRankine()    << " \u00B0R";  break;
            case Scale::REAUMUR:
                oss << asReaumur()    << " \u00B0Re"; break;
        }
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 3: CONVERSION RECORD (for history / logging)
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Immutable record of a single conversion event.
 *        Features: #31, #35, #38 (memory), #39 (undo buffer)
 */
struct ConversionRecord {
    long double    inputValue;
    std::string    inputUnit;
    long double    outputValue;
    std::string    outputUnit;
    std::string    timestamp;
    long long      durationMicros; ///< Feature #60: conversion speed
    int            points;         ///< Feature #62: gamification

    ConversionRecord(long double iv, const std::string& iu,
                     long double ov, const std::string& ou,
                     const std::string& ts, long long dur, int pts)
        : inputValue(iv), inputUnit(iu), outputValue(ov), outputUnit(ou),
          timestamp(ts), durationMicros(dur), points(pts) {}

    std::string toCSV() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6)
            << timestamp << ","
            << inputValue << ","  << inputUnit  << ","
            << outputValue << "," << outputUnit << ","
            << durationMicros << "us,"
            << points << " pts";
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 4: PERSISTENCE MANAGER
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Handles all File I/O: auto-save, CSV export, encrypted history,
 *        debug logging, leaderboard persistence.
 *        Features: #32, #33, #36, #42 (debug), #65 (leaderboard), #86 (encryption)
 */
class PersistenceManager {
private:
    std::string historyPath_;
    std::string csvPath_;
    std::string debugLogPath_;
    std::string leaderboardPath_;
    bool        autoSaveEnabled_;

    /**
     * @brief Simple XOR cipher for "encrypting" history file (Feature #86).
     *        Not cryptographically secure — purely demonstrative.
     */
    static std::string xorCipher(const std::string& data, char key = 0x5A) {
        std::string result = data;
        for (char& c : result) c ^= key;
        return result;
    }

    std::string currentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_ptr = std::localtime(&t);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
        return std::string(buf);
    }

public:
    explicit PersistenceManager(const std::string& dir = ".")
        : historyPath_(dir + "/temp_history.dat"),
          csvPath_(dir + "/temp_export.csv"),
          debugLogPath_(dir + "/temp_debug.log"),
          leaderboardPath_(dir + "/leaderboard.dat"),
          autoSaveEnabled_(true) {}

    bool isAutoSaveEnabled() const { return autoSaveEnabled_; }
    void setAutoSave(bool v)       { autoSaveEnabled_ = v; }

    /**
     * @brief Auto-saves a single record to the encrypted .dat file.
     *        Feature #36 (auto-save), Feature #86 (encryption).
     */
    void autoSave(const ConversionRecord& rec) {
        if (!autoSaveEnabled_) return;
        try {
            std::string line = rec.toCSV() + "\n";
            std::string encrypted = xorCipher(line);
            std::ofstream ofs(historyPath_, std::ios::app | std::ios::binary);
            if (!ofs) throw std::runtime_error("Cannot open history file: " + historyPath_);
            ofs.write(encrypted.c_str(), static_cast<std::streamsize>(encrypted.size()));
        } catch (const std::exception& e) {
            debugLog(std::string("AutoSave error: ") + e.what());
        }
    }

    /**
     * @brief Exports entire history to a readable CSV file (Feature #33).
     */
    void exportCSV(const std::vector<ConversionRecord>& history) {
        try {
            std::ofstream ofs(csvPath_);
            if (!ofs) throw std::runtime_error("Cannot create CSV: " + csvPath_);
            ofs << "Timestamp,InputValue,InputUnit,OutputValue,OutputUnit,Duration,Points\n";
            for (const auto& rec : history) {
                ofs << rec.toCSV() << "\n";
            }
            std::cout << Color::GREEN << "  CSV exported to: " << csvPath_ << Color::RESET << "\n";
        } catch (const std::exception& e) {
            std::cerr << Color::RED << "  Export error: " << e.what() << Color::RESET << "\n";
        }
    }

    /**
     * @brief Exports entire history to .txt (Feature #32).
     */
    void exportTXT(const std::vector<ConversionRecord>& history) {
        try {
            std::string txtPath = "temp_history.txt";
            std::ofstream ofs(txtPath);
            if (!ofs) throw std::runtime_error("Cannot create TXT: " + txtPath);
            ofs << "Temperature Conversion History\n";
            ofs << "Generated: " << currentTimestamp() << "\n";
            ofs << std::string(50, '=') << "\n\n";
            int i = 1;
            for (const auto& rec : history) {
                ofs << i++ << ". [" << rec.timestamp << "] "
                    << rec.inputValue << " " << rec.inputUnit
                    << " => "
                    << rec.outputValue << " " << rec.outputUnit
                    << " (" << rec.durationMicros << " us)\n";
            }
            std::cout << Color::GREEN << "  TXT exported to: " << txtPath << Color::RESET << "\n";
        } catch (const std::exception& e) {
            std::cerr << Color::RED << "  Export error: " << e.what() << Color::RESET << "\n";
        }
    }

    /**
     * @brief Writes debug info to hidden log file (Feature #82).
     */
    void debugLog(const std::string& msg) {
        try {
            std::ofstream ofs(debugLogPath_, std::ios::app);
            if (ofs) {
                ofs << "[" << currentTimestamp() << "] " << msg << "\n";
            }
        } catch (...) { /* silently ignore debug log failures */ }
    }

    /**
     * @brief Saves leaderboard (quiz high scores) to file (Feature #65).
     */
    void saveLeaderboard(const std::map<std::string, int>& scores) {
        try {
            std::ofstream ofs(leaderboardPath_);
            if (!ofs) throw std::runtime_error("Cannot write leaderboard");
            for (const auto& [name, score] : scores) {
                ofs << name << "," << score << "\n";
            }
        } catch (const std::exception& e) {
            debugLog(std::string("Leaderboard save error: ") + e.what());
        }
    }

    /**
     * @brief Loads leaderboard from file (Feature #65).
     */
    std::map<std::string, int> loadLeaderboard() {
        std::map<std::string, int> scores;
        try {
            std::ifstream ifs(leaderboardPath_);
            std::string line;
            while (std::getline(ifs, line)) {
                auto comma = line.find(',');
                if (comma != std::string::npos) {
                    std::string name = line.substr(0, comma);
                    int score = std::stoi(line.substr(comma + 1));
                    scores[name] = score;
                }
            }
        } catch (...) { /* return empty if file missing */ }
        return scores;
    }

    void clearHistory() {
        try {
            std::ofstream ofs(historyPath_, std::ios::trunc);
            std::ofstream cofs(csvPath_, std::ios::trunc);
            std::cout << Color::YELLOW << "  History cleared.\n" << Color::RESET;
        } catch (const std::exception& e) {
            debugLog(std::string("ClearHistory error: ") + e.what());
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 5: ANALYTICS ENGINE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Statistical and scientific computation module.
 *        Features: #11-#19, #67-#79
 */
class AnalyticsEngine {
public:
    /**
     * @brief Arithmetic mean of a list of Celsius values (Feature #18).
     */
    static double average(const std::vector<double>& values) {
        if (values.empty()) throw std::invalid_argument("Empty set for average.");
        double sum = 0;
        for (double v : values) sum += v;
        return sum / static_cast<double>(values.size());
    }

    /**
     * @brief Population standard deviation (Feature #19).
     */
    static double stdDev(const std::vector<double>& values) {
        if (values.size() < 2)
            throw std::invalid_argument("Need at least 2 values for std deviation.");
        double avg = average(values);
        double sumSq = 0;
        for (double v : values) sumSq += (v - avg) * (v - avg);
        return std::sqrt(sumSq / static_cast<double>(values.size()));
    }

    /**
     * @brief Heat energy Q = mcΔT (Feature #16).
     * @param mass_kg  Mass in kilograms
     * @param deltaT_C Temperature change in Celsius/Kelvin (same magnitude)
     * @param specificHeat J/(kg·K), defaults to water
     */
    static double heatEnergy(double mass_kg, double deltaT_C,
                             double specificHeat = Constants::SPECIFIC_HEAT_WATER) {
        if (mass_kg <= 0) throw std::invalid_argument("Mass must be positive.");
        return mass_kg * specificHeat * deltaT_C;
    }

    /**
     * @brief Dew point approximation using Magnus formula (Feature #77).
     * @param tempC     Air temperature in Celsius
     * @param humidity  Relative humidity 0-100
     */
    static double dewPoint(double tempC, double humidity) {
        if (humidity < 0 || humidity > 100)
            throw std::invalid_argument("Humidity must be 0-100.");
        const double a = 17.27, b = 237.7;
        double alpha = ((a * tempC) / (b + tempC)) + std::log(humidity / 100.0);
        return (b * alpha) / (a - alpha);
    }

    /**
     * @brief Wind chill index (Celsius, wind in km/h) — Feature #78.
     * @param tempC    Air temperature in Celsius
     * @param windKmh  Wind speed in km/h
     */
    static double windChill(double tempC, double windKmh) {
        if (windKmh < 0) throw std::invalid_argument("Wind speed cannot be negative.");
        // Environment Canada formula (valid for T <= 10°C, wind >= 3 km/h)
        return 13.12 + 0.6215 * tempC
               - 11.37 * std::pow(windKmh, 0.16)
               + 0.3965 * tempC * std::pow(windKmh, 0.16);
    }

    /**
     * @brief Heat index "feels like" temperature (Feature #76).
     * @param tempC    Air temperature in Celsius (>= 27°C for accuracy)
     * @param humidity Relative humidity 0-100
     */
    static double heatIndex(double tempC, double humidity) {
        // Rothfusz regression (converted from Fahrenheit formula)
        double T = tempC * 9.0 / 5.0 + 32.0; // convert to °F for formula
        double H = humidity;
        double HI = -42.379 + 2.04901523*T + 10.14333127*H
                    - 0.22475541*T*H - 0.00683783*T*T
                    - 0.05481717*H*H + 0.00122874*T*T*H
                    + 0.00085282*T*H*H - 0.00000199*T*T*H*H;
        return (HI - 32.0) * 5.0 / 9.0; // back to °C
    }

    /**
     * @brief Boiling point adjustment by altitude (Feature #79).
     *        Approximate: boiling point drops ~0.34°C per 100m.
     * @param altitudeM Altitude in meters
     */
    static double boilingPointAtAltitude(double altitudeM) {
        if (altitudeM < 0) throw std::invalid_argument("Altitude cannot be negative.");
        return 100.0 - (altitudeM / 100.0) * 0.34;
    }

    /**
     * @brief Gas law pressure ratio at two temperatures (Feature #17).
     *        P2/P1 = T2/T1 (Gay-Lussac's Law, constant volume)
     */
    static double pressureRatio(double t1_K, double t2_K) {
        if (t1_K <= 0) throw std::domain_error("Temperature must be positive Kelvin.");
        return t2_K / t1_K;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 6: TERMINAL UI ENGINE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Handles all terminal rendering: ASCII art, colors, boxes,
 *        progress bars, animations, clock display.
 *        Features: #21-#30, #43, #44, #87, #88, #89, #90
 */
class TerminalUI {
private:
    bool colorEnabled_;
    bool minimalistMode_;  ///< Feature #99
    int  language_;        ///< 0=English, 1=Hindi (transliterated), 2=Spanish — Feature #41
    bool darkMode_;        ///< Feature #44

    struct Lang {
        std::string welcome, converting, result, error, pressEnter;
    };

    const std::vector<Lang> languages_ = {
        {"Welcome", "Converting...", "Result", "Error", "Press Enter to continue"},
        {"Swagat hai", "Convert ho raha hai...", "Parinaam", "Galti", "Enter dabayein"},
        {"Bienvenido", "Convirtiendo...", "Resultado", "Error", "Presione Enter para continuar"}
    };

public:
    explicit TerminalUI(bool color = true) :
        colorEnabled_(color), minimalistMode_(false), language_(0), darkMode_(true) {}

    void setMinimalist(bool v) { minimalistMode_ = v; }
    void setLanguage(int l)    { language_ = (l >= 0 && l <= 2) ? l : 0; }
    void setDarkMode(bool v)   { darkMode_ = v; }
    bool isDarkMode()    const { return darkMode_; }
    bool isMinimalist()  const { return minimalistMode_; }

    std::string tr(const std::string& key) const {
        const auto& L = languages_[language_];
        if (key == "welcome")     return L.welcome;
        if (key == "converting")  return L.converting;
        if (key == "result")      return L.result;
        if (key == "error")       return L.error;
        if (key == "pressEnter")  return L.pressEnter;
        return key;
    }

    void clearScreen() const { system(CLEAR_SCREEN); }

    /**
     * @brief Renders the large ASCII Art header (Feature #21).
     */
    void printHeader() const {
        if (minimalistMode_) return;
        std::cout << Color::CYAN << Color::BOLD;
        std::cout << R"(
  ╔════════════════════════════════════════════════════════════╗
  ║  ████████╗███████╗███╗   ███╗██████╗                      ║
  ║     ██╔══╝██╔════╝████╗ ████║██╔══██╗                     ║
  ║     ██║   █████╗  ██╔████╔██║██████╔╝                     ║
  ║     ██║   ██╔══╝  ██║╚██╔╝██║██╔═══╝                      ║
  ║     ██║   ███████╗██║ ╚═╝ ██║██║                          ║
  ║     ╚═╝   ╚══════╝╚═╝     ╚═╝╚═╝  ANALYTICS SUITE  v2.0  ║
  ╚════════════════════════════════════════════════════════════╝
)" << Color::RESET;
        // Live clock in top area (Feature #49)
        printLiveClock();
    }

    /**
     * @brief Prints current date and time inline (Feature #49).
     */
    void printLiveClock() const {
        auto now  = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm*  tm_ptr = std::localtime(&t);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d  %H:%M:%S", tm_ptr);
        std::cout << Color::DIM << "  🕐 " << buf << Color::RESET << "\n\n";
    }

    /**
     * @brief Draws a double-line box around a block of text (Feature #25).
     */
    void printBox(const std::vector<std::string>& lines,
                  const std::string& borderColor = Color::CYAN) const {
        if (minimalistMode_) {
            for (const auto& l : lines) std::cout << l << "\n";
            return;
        }
        size_t maxLen = 0;
        for (const auto& l : lines) {
            // Strip ANSI codes for length calc
            size_t len = 0;
            bool inEsc = false;
            for (char c : l) {
                if (c == '\033') inEsc = true;
                else if (inEsc && c == 'm') inEsc = false;
                else if (!inEsc) len++;
            }
            maxLen = std::max(maxLen, len);
        }
        size_t width = maxLen + 4;
        std::cout << borderColor;
        std::cout << "  ╔" << std::string(width, '=') << "╗\n";
        for (const auto& l : lines) {
            // Count printable chars
            size_t len = 0;
            bool inEsc = false;
            for (char c : l) {
                if (c == '\033') inEsc = true;
                else if (inEsc && c == 'm') inEsc = false;
                else if (!inEsc) len++;
            }
            size_t padding = (width - 2 > len) ? (width - 2 - len) : 0;
            std::cout << "  ║ " << Color::RESET << l
                      << std::string(padding, ' ')
                      << borderColor << " ║\n";
        }
        std::cout << "  ╚" << std::string(width, '=') << "╝\n" << Color::RESET;
    }

    /**
     * @brief Renders a progress bar animation (Feature #23).
     * @param label  Label to show during loading
     * @param ms     Total milliseconds to animate
     */
    void progressBar(const std::string& label = "Converting", int ms = 300) const {
        if (minimalistMode_) return;
        std::cout << "\n  " << Color::YELLOW << label << ": [";
        std::cout.flush();
        int steps = 20;
        for (int i = 0; i < steps; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms / steps));
            std::cout << "█";
            std::cout.flush();
        }
        std::cout << "] " << Color::GREEN << "DONE" << Color::RESET << "\n\n";
    }

    /**
     * @brief Renders an ASCII thermometer with the current Celsius value (Feature #95).
     * @param celsius  Temperature in Celsius (-50 to 150 range)
     */
    void printThermometer(double celsius) const {
        if (minimalistMode_) return;
        // Map celsius to bar height (0–20 segments)
        auto clamp = [](double v, double lo, double hi) {
            return (v < lo) ? lo : (v > hi) ? hi : v;
        };
        double clamped  = clamp(celsius, -50.0, 150.0);
        int    segments = static_cast<int>((clamped + 50.0) / 200.0 * 20.0);
        std::string color = (celsius > 40) ? Color::RED :
                            (celsius < 5)  ? Color::BLUE : Color::GREEN;
        std::cout << "\n  " << Color::BOLD << "  ASCII Thermometer\n" << Color::RESET;
        std::cout << "  ┌──┐\n";
        for (int i = 20; i >= 1; i--) {
            std::string bar = (i <= segments) ? color + "████" + Color::RESET : "    ";
            std::cout << "  │" << bar << "│\n";
        }
        std::cout << "  └──┘\n";
        std::cout << "  " << color << std::fixed << std::setprecision(1)
                  << celsius << "°C" << Color::RESET << "\n\n";
    }

    /**
     * @brief Color-codes temperature (Red=hot, Blue=cold, Green=normal). Feature #22.
     */
    std::string tempColor(double celsius) const {
        if (celsius > 38.0)  return Color::RED;
        if (celsius < 0.0)   return Color::BLUE;
        return Color::GREEN;
    }

    /**
     * @brief Blinks text for extreme temperature warnings. Feature #26.
     */
    void printBlinkingAlert(const std::string& msg) const {
        if (minimalistMode_) { std::cout << "  !! " << msg << " !!\n"; return; }
        std::cout << Color::BLINK << Color::RED << "  ⚠  " << msg << "  ⚠"
                  << Color::RESET << "\n";
    }

    /**
     * @brief Emits an ASCII bell (beep) sound. Feature #27.
     */
    void beep() const {
        std::cout << '\a';
        std::cout.flush();
    }

    /**
     * @brief Prints the main menu. Feature #28 (menu-driven).
     */
    void printMainMenu(const std::string& userName, int totalPoints,
                       const std::string& level) const {
        if (!minimalistMode_) {
            std::cout << Color::CYAN << Color::BOLD;
            std::cout << "\n  ╔══════════════════════════════╗\n";
            std::cout << "  ║        MAIN  MENU            ║\n";
            std::cout << "  ╠══════════════════════════════╣\n";
        }
        auto item = [&](const std::string& num, const std::string& desc) {
            std::cout << "  ║  " << Color::YELLOW << num << Color::RESET
                      << "  " << desc;
            if (!minimalistMode_) {
                size_t pad = 24 - desc.size();
                if (pad > 100) pad = 0; // overflow guard
                std::cout << std::string(pad, ' ') << Color::CYAN << "║\n" << Color::RESET;
            } else {
                std::cout << "\n";
            }
        };
        item(" 1", "Single Conversion");
        item(" 2", "Multiple Inputs");
        item(" 3", "Range Table");
        item(" 4", "Science & Analytics");
        item(" 5", "Data Management");
        item(" 6", "System & Tools");
        item(" 7", "Games & Fun");
        item(" 8", "Real-World Context");
        item(" 9", "Settings");
        item("10", "Help / About");
        item(" Q", "Quit");
        if (!minimalistMode_) {
            std::cout << Color::CYAN << "  ╠══════════════════════════════╣\n";
            std::cout << "  ║  User: " << Color::GREEN << std::left << std::setw(10)
                      << userName.substr(0, 10)
                      << Color::CYAN << "  Pts:" << Color::YELLOW
                      << std::setw(5) << totalPoints
                      << Color::CYAN << "║\n";
            std::cout << "  ║  Level: " << Color::MAGENTA << std::left << std::setw(20)
                      << level
                      << Color::CYAN << "║\n";
            std::cout << "  ╚══════════════════════════════╝\n" << Color::RESET;
        }
    }

    /**
     * @brief Prints a range conversion table. Feature #7.
     */
    void printRangeTable(double startC, double endC, double step, int prec) const {
        std::cout << "\n";
        printBox({
            Color::BOLD + "  Celsius    │  Fahrenheit │  Kelvin    │  Rankine  " + Color::RESET,
            "  ──────────┼─────────────┼────────────┼───────────"
        });
        for (double c = startC; c <= endC + 1e-9; c += step) {
            double f = Converter<double>::celsiusToFahrenheit(c);
            double k = Converter<double>::celsiusToKelvin(c);
            double r = k * 9.0 / 5.0;
            std::string col = (c > 40) ? Color::RED : (c < 0) ? Color::BLUE : Color::GREEN;
            std::cout << "  " << col << std::fixed << std::setprecision(prec)
                      << std::setw(10) << c   << "°C │"
                      << std::setw(12) << f   << "°F │"
                      << std::setw(11) << k   << "K  │"
                      << std::setw(10) << r   << "°R"
                      << Color::RESET << "\n";
        }
        std::cout << "\n";
    }

    /**
     * @brief Prints a formatted single conversion result (Features #22, #25, #26).
     */
    void printResult(const Temperature& temp, int prec = 4) const {
        double c = static_cast<double>(temp.asCelsius());
        std::string col = tempColor(c);
        // Check extremes for alerts
        bool isBoiling  = c >= 99.5;
        bool isFreezing = c <= 0.1;
        bool isFever    = (c >= static_cast<double>(Constants::FEVER_THRESHOLD));

        std::vector<std::string> lines = {
            col + Color::BOLD + "  Conversion Result" + Color::RESET,
            "  " + std::string(30, '-'),
            "  Celsius    : " + col + temp.format(Temperature::Scale::CELSIUS)    + Color::RESET,
            "  Fahrenheit : " + col + temp.format(Temperature::Scale::FAHRENHEIT) + Color::RESET,
            "  Kelvin     : " + col + temp.format(Temperature::Scale::KELVIN)     + Color::RESET,
            "  Rankine    : " + col + temp.format(Temperature::Scale::RANKINE)    + Color::RESET,
            "  Réaumur    : " + col + temp.format(Temperature::Scale::REAUMUR)    + Color::RESET,
        };
        printBox(lines, col);

        if (isBoiling)  printBlinkingAlert("Water boils at this temperature!");
        if (isFreezing) printBlinkingAlert("Water freezes at this temperature!");
        if (isFever)    printBlinkingAlert("Fever range! Seek medical attention.");
        if (isBoiling || isFreezing) beep();
        printThermometer(c);
    }

    /**
     * @brief Prints interactive help. Feature #30.
     */
    void printHelp() const {
        printBox({
            Color::BOLD + "  HELP — Keyboard Shortcuts & Commands" + Color::RESET,
            "  Q / q      : Quit program",
            "  C / c      : Clear screen",
            "  M / m      : Recall last conversion (Memory)",
            "  U / u      : Undo last conversion",
            "  --help     : Show this help",
            "  ./temp_suite 32 -f   : Command-line mode",
            "  Scales accepted: C, F, K, R (Rankine), Re (Réaumur)",
            "  Auto-detect: typing '25C' or '77F' is understood"
        });
    }

    /**
     * @brief Prints a science fact of the day. Feature #64.
     */
    void printFactOfTheDay() const {
        static const std::vector<std::string> facts = {
            "Absolute zero (0 K) is the coldest possible temperature.",
            "The surface of the Sun is ~5,778 K (5,504°C).",
            "Pluto's surface can reach -233°C (-387°F).",
            "Lightning channels reach ~30,000 K — 5× hotter than the Sun's surface.",
            "Human body core temp is tightly regulated at ~37°C (98.6°F).",
            "Tungsten melts at 3,422°C — the highest melting point of any pure metal.",
            "Helium remains liquid at 1 atm pressure all the way to absolute zero.",
            "The Celsius scale was originally inverted (100=freezing, 0=boiling).",
            "Anders Celsius defined his scale in 1742; Fahrenheit in 1724.",
            "The cosmic microwave background radiation is ~2.73 K.",
        };
        std::mt19937 rng(static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<size_t> dist(0, facts.size() - 1);
        printBox({"  🔬 Fact of the Day:", "  " + facts[dist(rng)]}, Color::MAGENTA);
    }

    /**
     * @brief Prints a footer quote. Feature #93.
     */
    void printFooter() const {
        if (minimalistMode_) return;
        static const std::vector<std::string> quotes = {
            "\"The scientist does not study nature because it is useful; he studies it because he delights in it.\" — H. Poincaré",
            "\"In physics, you don't have to go around making trouble for yourself; nature does it for you.\" — F. Wilczek",
            "\"Equipped with his five senses, man explores the universe.\" — E. Hubble",
        };
        std::mt19937 rng(static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_int_distribution<size_t> dist(0, quotes.size() - 1);
        std::cout << Color::DIM << "\n  " << quotes[dist(rng)] << "\n" << Color::RESET;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 7: USER PROFILE & GAMIFICATION
// ═══════════════════════════════════════════════════════════════

/**
 * @brief User profile, gamification, session tracking.
 *        Features: #42 (profile), #61 (points), #62 (achievements),
 *                  #66 (level), #67 (easter eggs), #34 (session timer),
 *                  #91 (coffee break reminder)
 */
class UserProfile {
private:
    std::string name_;
    int         totalPoints_;
    int         conversionCount_;
    std::vector<std::string> achievements_;
    std::chrono::steady_clock::time_point sessionStart_;

    std::string computeLevel() const {
        if (totalPoints_ < 50)    return "Beginner 🌱";
        if (totalPoints_ < 200)   return "Student 📚";
        if (totalPoints_ < 500)   return "Practitioner ⚗️";
        if (totalPoints_ < 1000)  return "Scientist 🔬";
        return "Nobel Laureate 🏆";
    }

public:
    explicit UserProfile(const std::string& name = "Guest")
        : name_(name), totalPoints_(0), conversionCount_(0),
          sessionStart_(std::chrono::steady_clock::now()) {}

    void setName(const std::string& n) { name_ = n; }
    std::string getName()  const { return name_; }
    int  getPoints()       const { return totalPoints_; }
    int  getConversionCount() const { return conversionCount_; }
    std::string getLevel() const { return computeLevel(); }

    /**
     * @brief Add points per conversion. Feature #61.
     */
    int addConversion() {
        ++conversionCount_;
        int pts = 10;
        totalPoints_ += pts;
        // Achievement checks (Feature #62)
        if (conversionCount_ == 1)
            achievements_.push_back("🎖 First Conversion!");
        if (conversionCount_ == 10)
            achievements_.push_back("⭐ 10 Conversions Done!");
        if (conversionCount_ == 50)
            achievements_.push_back("🔥 50 Conversions — On Fire!");
        if (conversionCount_ == 100)
            achievements_.push_back("🏆 Century Club!");
        return pts;
    }

    /**
     * @brief Feature #91: coffee break reminder after 30 minutes.
     */
    bool needsCoffeeBreak() const {
        auto elapsed = std::chrono::steady_clock::now() - sessionStart_;
        return std::chrono::duration_cast<std::chrono::minutes>(elapsed).count() >= 30;
    }

    /**
     * @brief Feature #34: session elapsed time string.
     */
    std::string sessionDuration() const {
        auto elapsed = std::chrono::steady_clock::now() - sessionStart_;
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        std::ostringstream oss;
        oss << secs / 60 << "m " << secs % 60 << "s";
        return oss.str();
    }

    /**
     * @brief Easter egg detector (Feature #67).
     */
    static std::string easterEgg(double value) {
        int iv = static_cast<int>(std::round(value));
        if (iv == 69)  return "  😏 Nice.";
        if (iv == 420) return "  🌿 Blaze it (at 420°... that would vaporize most things).";
        if (iv == 0)   return "  🧊 Absolute cool. Water freezes here.";
        if (iv == 100) return "  💧 Water's boiling point at sea level!";
        return "";
    }

    /**
     * @brief Print any newly earned achievements. Feature #62.
     */
    void printNewAchievements(const TerminalUI& ui) const {
        // Print only the last one earned (most recent)
        if (!achievements_.empty()) {
            ui.printBox({Color::YELLOW + "  Achievement Unlocked! " + Color::RESET,
                         "  " + achievements_.back()}, Color::YELLOW);
        }
    }

    void printAllAchievements() const {
        std::cout << Color::BOLD << "\n  Achievements:\n" << Color::RESET;
        if (achievements_.empty()) {
            std::cout << "  (None yet — start converting!)\n";
        }
        for (const auto& a : achievements_) {
            std::cout << "    " << a << "\n";
        }
    }

    void printSummary() const {
        std::cout << Color::BOLD << "\n  ── Session Summary ──\n" << Color::RESET;
        std::cout << "  User         : " << name_ << "\n";
        std::cout << "  Conversions  : " << conversionCount_ << "\n";
        std::cout << "  Total Points : " << totalPoints_ << "\n";
        std::cout << "  Level        : " << computeLevel() << "\n";
        std::cout << "  Session Time : " << sessionDuration() << "\n\n";
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 8: QUIZ ENGINE (Gamification)
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Daily quiz and leaderboard. Features: #61, #64, #65, #68, #69.
 */
class QuizEngine {
private:
    std::map<std::string, int>& leaderboard_;
    PersistenceManager&         pm_;

public:
    QuizEngine(std::map<std::string, int>& lb, PersistenceManager& pm)
        : leaderboard_(lb), pm_(pm) {}

    /**
     * @brief Runs one quiz round — guess Fahrenheit for a given Celsius. Feature #61.
     */
    int runQuiz(const std::string& playerName, const TerminalUI& ui) {
        std::mt19937 rng(static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        std::uniform_real_distribution<double> dist(-20.0, 120.0);
        double celsius = std::round(dist(rng));
        double correct = std::round(Converter<double>::celsiusToFahrenheit(celsius) * 10) / 10;

        std::cout << Color::YELLOW << "\n  🎮 TEMPERATURE QUIZ\n" << Color::RESET;
        std::cout << "  What is " << celsius << "°C in Fahrenheit?\n";
        std::cout << "  Your answer: ";
        double answer;
        if (!(std::cin >> answer)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return 0;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        double tolerance = 1.0;
        int pts = 0;
        if (std::abs(answer - correct) <= tolerance) {
            std::cout << Color::GREEN << "  ✓ Correct! +" << 25 << " points\n" << Color::RESET;
            pts = 25;
        } else {
            std::cout << Color::RED << "  ✗ Wrong. Correct answer: " << correct << "°F\n" << Color::RESET;
        }
        leaderboard_[playerName] += pts;
        pm_.saveLeaderboard(leaderboard_);
        return pts;
    }

    void printLeaderboard() const {
        std::cout << Color::BOLD << "\n  🏆 LEADERBOARD\n" << Color::RESET;
        if (leaderboard_.empty()) {
            std::cout << "  (No scores yet.)\n";
            return;
        }
        // Sort by score descending
        std::vector<std::pair<std::string, int>> sorted(
            leaderboard_.begin(), leaderboard_.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b){ return a.second > b.second; });
        int rank = 1;
        for (const auto& [name, score] : sorted) {
            std::cout << "  " << rank++ << ". " << std::left << std::setw(15)
                      << name << " → " << score << " pts\n";
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 9: SYSTEM INFO MODULE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Gathers and displays system information.
 *        Features: #47 (battery), #50 (CPU temp), #83 (memory),
 *                  #84 (compiler), #86 (architecture).
 */
class SystemInfo {
public:
    /**
     * @brief Architecture detection (Feature #86).
     */
    static std::string architecture() {
        std::ostringstream oss;
        oss << ARCH_BITS << "-bit";
        return oss.str();
    }

    /**
     * @brief Compiler version info (Feature #84).
     */
    static std::string compilerInfo() {
        std::ostringstream oss;
#if defined(__GNUC__)
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(__clang__)
        oss << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(_MSC_VER)
        oss << "MSVC " << _MSC_VER;
#else
        oss << "Unknown Compiler";
#endif
        oss << "  |  C++ Standard: " << __cplusplus;
        return oss.str();
    }

    /**
     * @brief Rough memory usage via /proc/self/status (Linux). Feature #83.
     */
    static std::string memoryUsage() {
#ifdef PLATFORM_LINUX
        std::ifstream ifs("/proc/self/status");
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.find("VmRSS") != std::string::npos) {
                return line;
            }
        }
#endif
        return "Memory info unavailable on this platform.";
    }

    /**
     * @brief CPU temperature (Linux /sys). Feature #50.
     */
    static std::string cpuTemperature() {
#ifdef PLATFORM_LINUX
        std::ifstream ifs("/sys/class/thermal/thermal_zone0/temp");
        if (ifs) {
            int millidegrees;
            ifs >> millidegrees;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1)
                << (millidegrees / 1000.0) << "°C";
            return oss.str();
        }
#endif
        return "N/A (not Linux, or no sensor)";
    }

    /**
     * @brief Fake battery status — demonstrates the concept. Feature #47.
     */
    static std::string batteryStatus() {
#ifdef PLATFORM_LINUX
        std::ifstream cap("/sys/class/power_supply/BAT0/capacity");
        std::ifstream status("/sys/class/power_supply/BAT0/status");
        if (cap) {
            int pct; cap >> pct;
            std::string st; if (status) status >> st;
            return std::to_string(pct) + "% (" + st + ")";
        }
#endif
        return "Battery info unavailable.";
    }

    /**
     * @brief Benchmark: 100,000 double conversions. Feature #85.
     */
    static void runBenchmark() {
        std::cout << "\n  Running benchmark (100,000 C→F conversions)...\n";
        auto t1 = std::chrono::high_resolution_clock::now();
        volatile double acc = 0;
        for (int i = 0; i < 100000; i++) {
            acc += Converter<double>::celsiusToFahrenheit(static_cast<double>(i % 500));
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::cout << Color::GREEN
                  << "  Done in " << std::fixed << std::setprecision(3) << ms << " ms"
                  << "  (acc=" << acc << ")\n" << Color::RESET;
    }

    /**
     * @brief Hex/Binary output of float bits. Feature #87.
     */
    static void printBinaryRepresentation(double value) {
        uint64_t bits;
        static_assert(sizeof(double) == sizeof(uint64_t), "");
        std::memcpy(&bits, &value, sizeof(bits));
        std::cout << "  Double " << value << " in bits:\n";
        std::cout << "    HEX: 0x" << std::uppercase << std::hex << bits << "\n";
        std::cout << std::dec;
        std::cout << "    BIN: ";
        for (int i = 63; i >= 0; i--) {
            std::cout << ((bits >> i) & 1);
            if (i == 63 || i == 52) std::cout << "|";
        }
        std::cout << "\n";
        std::cout << "    (Sign|Exponent(11)|Mantissa(52))\n";
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 10: INPUT PARSER
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Robust input handling: trimming, auto-detection, validation.
 *        Features: #8 (auto-detect scale), #51-#60 (input safety)
 */
class InputParser {
public:
    struct ParsedInput {
        double      value;
        std::string unit;   // "C", "F", "K", "R", "Re"
        bool        valid;
        std::string errorMsg;
    };

    /**
     * @brief Trims leading/trailing whitespace. Feature #60.
     */
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    /**
     * @brief Auto-detects scale from string like "25C", "77F", "300K". Feature #8.
     */
    static ParsedInput parse(const std::string& raw) {
        ParsedInput result{0.0, "C", false, ""};
        std::string s = trim(raw);

        if (s.empty()) {
            result.errorMsg = "Empty input — please enter a value.";  // Feature #53
            return result;
        }

        // Feature #57 — character limit
        if (s.size() > 30) {
            result.errorMsg = "Input too long (max 30 chars).";
            return result;
        }

        // Detect trailing unit suffix (case-insensitive). Feature #8, #58
        std::string numStr = s;
        std::string unit   = "C"; // default

        // Check for "Re" (Réaumur) first (two chars)
        if (s.size() >= 2) {
            std::string tail2 = s.substr(s.size() - 2);
            // normalize
            if (tail2 == "re" || tail2 == "Re" || tail2 == "RE")
                { numStr = s.substr(0, s.size() - 2); unit = "Re"; }
        }
        if (unit == "C" && !s.empty()) {
            char last = static_cast<char>(std::toupper(static_cast<unsigned char>(s.back())));
            if (last == 'C') { numStr = s.substr(0, s.size()-1); unit = "C"; }
            else if (last == 'F') { numStr = s.substr(0, s.size()-1); unit = "F"; }
            else if (last == 'K') { numStr = s.substr(0, s.size()-1); unit = "K"; }
            else if (last == 'R') { numStr = s.substr(0, s.size()-1); unit = "R"; }
        }

        // Feature #52 — non-numeric check
        try {
            size_t pos;
            double val = std::stod(trim(numStr), &pos);
            if (pos != trim(numStr).size()) {
                result.errorMsg = "Non-numeric characters detected. Please enter a number.";
                return result;
            }

            // Feature #55 — overflow protection
            if (std::isinf(val) || std::isnan(val) ||
                val > 1e15 || val < -1e15) {
                result.errorMsg = "Value out of representable range (overflow/underflow).";
                return result;
            }

            result.value = val;
            result.unit  = unit;
            result.valid = true;
        } catch (const std::invalid_argument&) {
            result.errorMsg = "Invalid number format. (e.g., use: 25, -10.5, 37.5C)";
        } catch (const std::out_of_range&) {
            result.errorMsg = "Number is out of range.";
        }
        return result;
    }

    /**
     * @brief Reads a line with retry loop. Features #52, #56 (retry loop).
     */
    static std::string readLine(const std::string& prompt) {
        for (int attempt = 0; attempt < 5; attempt++) {
            std::cout << prompt;
            std::string line;
            if (std::getline(std::cin, line)) {
                return trim(line);
            }
            // Handle stream error / EOF
            if (std::cin.eof()) return "Q"; // Treat EOF as quit
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return "Q";
    }

    /**
     * @brief Reads a double with retry loop and validation. Feature #56.
     */
    static std::optional<double> readDouble(const std::string& prompt,
                                             double lo = -1e15, double hi = 1e15) {
        for (int attempt = 0; attempt < 5; attempt++) {
            std::string raw = readLine(prompt);
            if (raw == "Q" || raw == "q") return std::nullopt;
            auto res = parse(raw);
            if (res.valid && res.value >= lo && res.value <= hi) return res.value;
            std::cout << Color::RED << "  ⚠  "
                      << (res.valid ? "Value out of range." : res.errorMsg)
                      << " Try again.\n" << Color::RESET;
        }
        return std::nullopt;
    }

    /**
     * @brief Feature #57: exits confirmation. Feature #57.
     */
    static bool confirmExit() {
        std::cout << Color::YELLOW
                  << "\n  Are you sure you want to exit? (y/N): "
                  << Color::RESET;
        std::string s = readLine("");
        return (s == "y" || s == "Y");
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 11: REAL-WORLD CONTEXT MODULE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Provides contextual info: weather, cooking, medical, NASA.
 *        Features: #71-#80
 */
class ContextModule {
public:
    /**
     * @brief Travel clothing advice based on temperature. Feature #75.
     */
    static std::string travelTip(double celsius) {
        if (celsius < -20) return "🧣 Extreme cold! Thermal layers, gloves, balaclava.";
        if (celsius < 0)   return "🧥 Very cold! Heavy winter coat, scarf, gloves.";
        if (celsius < 10)  return "🧥 Cold. Warm jacket needed.";
        if (celsius < 18)  return "🧤 Cool. Light jacket or sweater.";
        if (celsius < 25)  return "👕 Comfortable. T-shirt weather.";
        if (celsius < 35)  return "☀️  Warm. Light clothing, stay hydrated.";
        return "🌡 Very hot! Stay indoors, drink plenty of water.";
    }

    /**
     * @brief Medical reference for body temperature. Feature #74.
     */
    static std::string medicalContext(double celsius) {
        if (celsius < 35.0) return "🚨 Hypothermia risk! Seek emergency medical help.";
        if (celsius < 36.1) return "🥶 Slightly below normal body temp (36.1-37.2°C is normal).";
        if (celsius <= 37.2)return "✅ Normal body temperature range.";
        if (celsius <= 38.0)return "🌡 Slightly elevated — could be early fever. Monitor.";
        if (celsius <= 39.0)return "🤒 Fever (adults). Rest and hydration advised.";
        if (celsius <= 39.5)return "🤒 Moderate fever. Consider medical consultation.";
        return "🚨 High fever (>39.5°C)! Seek medical attention immediately.";
    }

    /**
     * @brief Cooking / oven guide. Feature #73.
     */
    static std::string cookingGuide(double celsius) {
        if (celsius < 100)  return "Below boiling — suitable for poaching.";
        if (celsius < 150)  return "Low oven (~300°F) — slow baking, meringues.";
        if (celsius < 180)  return "Moderate oven (~350°F) — cakes, cookies.";
        if (celsius < 200)  return "Moderate-high (~400°F) — breads, roasts.";
        if (celsius < 220)  return "Hot oven (~425°F) — pizza, crusty bread.";
        if (celsius < 250)  return "Very hot (~480°F) — Neapolitan pizza.";
        return "Extreme heat — beyond typical oven range.";
    }

    /**
     * @brief Hardcoded city temperatures (Feature #71 — weather forecast simulation).
     */
    static void printCityWeather() {
        static const std::map<std::string, double> cities = {
            {"New Delhi, IN",  38.0}, {"Mumbai, IN",    32.0},
            {"London, UK",     14.0}, {"New York, USA", 22.0},
            {"Moscow, RU",      4.0}, {"Sydney, AU",    20.0},
            {"Dubai, UAE",     42.0}, {"Tokyo, JP",     25.0},
        };
        std::cout << Color::BOLD << "\n  🌍 City Weather Snapshot\n" << Color::RESET;
        for (const auto& [city, temp] : cities) {
            std::string col = (temp > 35) ? Color::RED :
                              (temp < 10) ? Color::BLUE : Color::GREEN;
            double f = Converter<double>::celsiusToFahrenheit(temp);
            std::cout << "  " << std::left << std::setw(20) << city
                      << col << std::fixed << std::setprecision(1)
                      << temp << "°C  (" << f << "°F)"
                      << Color::RESET << "\n";
        }
    }

    /**
     * @brief NASA-mode: space temperatures. Feature #80.
     */
    static void printNASAMode() {
        std::cout << Color::BOLD << Color::CYAN
                  << "\n  🚀 NASA Temperature Reference\n" << Color::RESET;
        static const std::vector<std::pair<std::string, double>> objects = {
            {"Sun Surface",          5504.0},
            {"Venus Surface",         464.0},
            {"Earth Average",          15.0},
            {"Mars Surface (avg)",    -63.0},
            {"Moon (day)",            127.0},
            {"Moon (night)",         -173.0},
            {"Pluto Surface",        -233.0},
            {"Cosmic Background", -270.42},
        };
        for (const auto& [obj, c] : objects) {
            double f = Converter<double>::celsiusToFahrenheit(c);
            double k = Converter<double>::celsiusToKelvin(c);
            std::cout << "  " << std::left << std::setw(22) << obj
                      << std::fixed << std::setprecision(1)
                      << std::setw(12) << c  << "°C"
                      << std::setw(12) << f  << "°F"
                      << std::setw(10) << k  << "K\n";
        }
    }

    /**
     * @brief Climate change facts. Feature #72.
     */
    static void printClimateInfo() {
        std::cout << Color::YELLOW << "\n  🌿 Climate Change Facts\n" << Color::RESET;
        std::cout << "  • Global average temperature has risen ~1.1°C since pre-industrial times.\n";
        std::cout << "  • The 2015 Paris Agreement targets limiting warming to 1.5–2°C.\n";
        std::cout << "  • Arctic sea ice is declining ~13% per decade.\n";
        std::cout << "  • Ocean heat content has increased every decade since the 1970s.\n";
        std::cout << "  • CO2 levels are now above 420 ppm — highest in 3 million years.\n";
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 12: DEVELOPER TOOLS MODULE
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Verbose mode, API simulation, plugin stub, version history.
 *        Features: #81 (verbose), #45 (API sim), #46 (update check),
 *                  #88 (plugin), #89 (reset), #90 (version log)
 */
class DevTools {
public:
    /**
     * @brief Verbose conversion — shows intermediate steps. Feature #81.
     */
    static void verboseConvert(double value, const std::string& fromUnit) {
        std::cout << Color::DIM << "\n  [Verbose Mode]\n";
        std::cout << "  Step 1: Input = " << value << " " << fromUnit << "\n";
        double kelvin;
        if      (fromUnit == "C") { kelvin = Converter<double>::celsiusToKelvin(value);
                                    std::cout << "  Step 2: C → K: " << value << " + 273.15 = " << kelvin << " K\n"; }
        else if (fromUnit == "F") { double c = Converter<double>::fahrenheitToCelsius(value);
                                    kelvin   = Converter<double>::celsiusToKelvin(c);
                                    std::cout << "  Step 2a: F → C: (" << value << " - 32) × 5/9 = " << c << "°C\n";
                                    std::cout << "  Step 2b: C → K: " << c << " + 273.15 = " << kelvin << " K\n"; }
        else                      { kelvin = value;
                                    std::cout << "  Step 2: Already in Kelvin = " << kelvin << "\n"; }
        double celsius = Converter<double>::kelvinToCelsius(kelvin);
        double fahr    = Converter<double>::kelvinToFahrenheit(kelvin);
        std::cout << "  Step 3: K → C: " << kelvin << " - 273.15 = " << celsius << "°C\n";
        std::cout << "  Step 4: K → F: " << kelvin << " × 9/5 - 459.67 = " << fahr << "°F\n";
        std::cout << Color::RESET;
    }

    /**
     * @brief Fake JSON API response output. Feature #45.
     */
    static void printAPISimulation(double valueC) {
        double f = Converter<double>::celsiusToFahrenheit(valueC);
        double k = Converter<double>::celsiusToKelvin(valueC);
        double r = k * 9.0 / 5.0;
        std::cout << Color::GREEN << "\n  // API Response (JSON simulation)\n";
        std::cout << "  {\n";
        std::cout << "    \"status\": \"success\",\n";
        std::cout << "    \"input\": { \"value\": " << valueC << ", \"unit\": \"Celsius\" },\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "    \"results\": {\n";
        std::cout << "      \"fahrenheit\": " << f << ",\n";
        std::cout << "      \"kelvin\": "     << k << ",\n";
        std::cout << "      \"rankine\": "    << r << "\n";
        std::cout << "    },\n";
        std::cout << "    \"meta\": {\n";
        std::cout << "      \"precision\": \"long double\",\n";
        std::cout << "      \"version\": \"2.0.0\"\n";
        std::cout << "    }\n";
        std::cout << "  }\n" << Color::RESET;
    }

    /**
     * @brief Fake update checker. Feature #46.
     */
    static void checkForUpdates() {
        std::cout << Color::CYAN << "\n  🔄 Checking for updates";
        for (int i = 0; i < 3; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << ".";
            std::cout.flush();
        }
        std::cout << "\n  ✅ You are running the latest version: v2.0.0\n" << Color::RESET;
    }

    /**
     * @brief Version history / changelog. Feature #90.
     */
    static void printVersionHistory() {
        std::cout << Color::BOLD << "\n  Version History\n" << Color::RESET;
        std::cout << "  v2.0.0 — 100-feature professional suite, C++17, templates\n";
        std::cout << "  v1.5.0 — Added gamification, quiz, leaderboard\n";
        std::cout << "  v1.2.0 — CSV export, encryption, analytics engine\n";
        std::cout << "  v1.0.0 — Initial release: C/F/K conversion\n";
    }

    /**
     * @brief Prints fake social-share clipboard text. Feature #94.
     */
    static void printSocialShare(double valueC, double valueF) {
        std::cout << Color::MAGENTA
                  << "\n  📋 Share-ready format (copy below):\n"
                  << "  ─────────────────────────────────────\n"
                  << "  " << valueC << "°C = " << valueF << "°F"
                  << " | Converted with TempSuite v2.0\n"
                  << "  ─────────────────────────────────────\n"
                  << Color::RESET;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 13: APPLICATION CLASS (Main Controller)
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Root application class. Orchestrates all subsystems.
 *        Implements Command-pattern style menu dispatch.
 *        Features: all remaining (#41-#100)
 */
class Application {
private:
    // Smart-pointer ownership of subsystems (zero-leak policy)
    std::unique_ptr<TerminalUI>        ui_;
    std::unique_ptr<PersistenceManager> pm_;
    std::unique_ptr<UserProfile>        user_;

    std::vector<ConversionRecord>       history_;   ///< Feature #31 (history log)
    std::vector<ConversionRecord>       undoBuffer_;///< Feature #39 (undo)
    std::optional<ConversionRecord>     memory_;    ///< Feature #38 (memory)

    std::map<std::string, int>          leaderboard_;
    std::unique_ptr<QuizEngine>         quiz_;

    bool verboseMode_  = false; ///< Feature #81
    bool running_      = true;
    int  defaultPrec_  = 4;

    // ── Timestamp helper ──────────────────────────────────────
    std::string timestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_ptr = std::localtime(&t);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
        return std::string(buf);
    }

    // ── Core conversion driver ────────────────────────────────
    /**
     * @brief Performs a conversion with full timing, logging, gamification.
     * Features: #1-#9, #31, #34-#36, #38-#39, #60-#62
     */
    void doConversion(double value, const std::string& unit) {
        auto t_start = std::chrono::high_resolution_clock::now();

        // Map unit string to Scale enum
        Temperature::Scale scale = Temperature::Scale::CELSIUS;
        std::string unitFull = "°C";
        if (unit == "F") { scale = Temperature::Scale::FAHRENHEIT; unitFull = "°F"; }
        else if (unit == "K") { scale = Temperature::Scale::KELVIN;  unitFull = "K";  }
        else if (unit == "R") { scale = Temperature::Scale::RANKINE; unitFull = "°R"; }
        else if (unit == "Re"){ scale = Temperature::Scale::REAUMUR; unitFull = "°Re";}

        Temperature temp(value, scale, defaultPrec_);

        // Verbose mode details (Feature #81)
        if (verboseMode_) DevTools::verboseConvert(value, unit);

        // Animated progress bar (Feature #23)
        ui_->progressBar("Computing", 200);

        // Display result (Features #22, #25, #26)
        ui_->printResult(temp, defaultPrec_);

        // Contextual info
        double c = static_cast<double>(temp.asCelsius());
        std::cout << "  🌡 Medical : " << ContextModule::medicalContext(c) << "\n";
        std::cout << "  👕 Travel  : " << ContextModule::travelTip(c)     << "\n";
        std::cout << "  🍳 Cooking : " << ContextModule::cookingGuide(c)  << "\n";

        // Absolute zero check (Feature #9)
        if (c < static_cast<double>(Constants::ABSOLUTE_ZERO_C) + 0.001)
            ui_->printBlinkingAlert("BELOW ABSOLUTE ZERO — unphysical value!");

        // Easter egg (Feature #67)
        std::string egg = UserProfile::easterEgg(value);
        if (!egg.empty()) std::cout << Color::MAGENTA << egg << Color::RESET << "\n";

        // Room temp comparison (Feature #13)
        double diff = c - static_cast<double>(Constants::ROOM_TEMP_C);
        std::cout << "  📊 vs Room Temp (22°C): " << std::showpos << std::fixed
                  << std::setprecision(2) << diff << "°C"
                  << std::noshowpos << "\n";

        auto t_end = std::chrono::high_resolution_clock::now();
        long long micros = std::chrono::duration_cast<std::chrono::microseconds>(
                               t_end - t_start).count();

        // Feature #60 — conversion speed
        std::cout << "  ⚡ Computed in " << micros << " μs\n";

        // Gamification (Feature #61)
        int pts = user_->addConversion();
        std::cout << "  🏅 +" << pts << " points  (Total: " << user_->getPoints() << ")\n";
        user_->printNewAchievements(*ui_);

        // Record in history (Feature #31, #35)
        double outVal = static_cast<double>(temp.asFahrenheit());
        ConversionRecord rec(value, unitFull, outVal, "°F",
                             timestamp(), micros, pts);
        history_.push_back(rec);
        if (history_.size() > static_cast<size_t>(Constants::MAX_HISTORY))
            history_.erase(history_.begin()); // FIFO trim

        undoBuffer_.push_back(rec);           // Feature #39 (undo)
        memory_ = rec;                        // Feature #38 (memory)
        pm_->autoSave(rec);                   // Feature #36 (auto-save)

        // Coffee break check (Feature #91)
        if (user_->needsCoffeeBreak()) {
            ui_->printBox({"☕  Coffee Break Reminder!",
                           "You've been using TempSuite for 30+ minutes.",
                           "Stretch, hydrate, and take a break!"}, Color::YELLOW);
        }
    }

    // ── Menu Handlers ─────────────────────────────────────────

    void menuSingleConversion() {
        ui_->clearScreen();
        ui_->printHeader();
        std::cout << Color::BOLD << "  ─── Single Conversion ───\n" << Color::RESET;
        std::cout << "  Enter temperature (e.g., 37, 37C, 98.6F, 300K): ";
        std::string raw = InputParser::readLine("");
        // Feature #57 (Q to quit)
        if (raw == "Q" || raw == "q") return;

        auto parsed = InputParser::parse(raw);
        if (!parsed.valid) {
            std::cout << Color::RED << "  ⚠  " << parsed.errorMsg << Color::RESET << "\n";
            std::cout << "  Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        try {
            doConversion(parsed.value, parsed.unit);
        } catch (const std::domain_error& e) {
            ui_->printBlinkingAlert(e.what());
        } catch (const std::exception& e) {
            std::cout << Color::RED << "  Error: " << e.what() << Color::RESET << "\n";
            pm_->debugLog(std::string("Conversion error: ") + e.what());
        }
        std::cout << "\n  Press Enter to continue...";
        InputParser::readLine("");
    }

    void menuMultipleInputs() {
        ui_->clearScreen();
        ui_->printHeader();
        std::cout << Color::BOLD << "  ─── Multiple Inputs ───\n" << Color::RESET;
        std::cout << "  Enter values separated by commas (e.g., 0, 25, 37, 100):\n  > ";
        std::string line = InputParser::readLine("");
        if (line.empty() || line == "Q") return;

        std::istringstream iss(line);
        std::string token;
        std::vector<double> values;
        while (std::getline(iss, token, ',')) {
            auto parsed = InputParser::parse(token);
            if (parsed.valid) values.push_back(parsed.value);
        }
        if (values.empty()) {
            std::cout << Color::RED << "  No valid values found.\n" << Color::RESET;
            InputParser::readLine("");
            return;
        }
        for (double v : values) {
            std::cout << "\n  ── " << v << "°C ──\n";
            try { doConversion(v, "C"); }
            catch (const std::exception& e) {
                std::cout << Color::RED << "  Skipping " << v << ": " << e.what()
                          << Color::RESET << "\n";
            }
        }

        // Stats (Features #18, #19)
        try {
            double avg = AnalyticsEngine::average(values);
            double sd  = AnalyticsEngine::stdDev(values);
            std::cout << Color::BOLD << "\n  ── Statistics ──\n" << Color::RESET;
            std::cout << "  Average : " << std::fixed << std::setprecision(2) << avg << "°C\n";
            std::cout << "  Std Dev : " << sd << "°C\n";
        } catch (const std::exception& e) {
            std::cout << "  (Stats unavailable: " << e.what() << ")\n";
        }
        InputParser::readLine("\n  Press Enter...");
    }

    void menuRangeTable() {
        ui_->clearScreen();
        ui_->printHeader();
        std::cout << Color::BOLD << "  ─── Range Conversion Table ───\n" << Color::RESET;
        auto start = InputParser::readDouble("  Start °C: ", -273.15, 10000);
        if (!start) return;
        auto end   = InputParser::readDouble("  End   °C: ", *start, 10000);
        if (!end)   return;
        auto step  = InputParser::readDouble("  Step  °C: ", 0.001, 1000);
        if (!step)  return;
        ui_->printRangeTable(*start, *end, *step, defaultPrec_);
        InputParser::readLine("  Press Enter...");
    }

    void menuScience() {
        ui_->clearScreen();
        ui_->printHeader();
        std::cout << Color::BOLD << "  ─── Science & Analytics ───\n" << Color::RESET;
        std::cout << "  1. Heat Energy (Q = mcΔT)\n";
        std::cout << "  2. Dew Point\n";
        std::cout << "  3. Wind Chill\n";
        std::cout << "  4. Heat Index (feels-like)\n";
        std::cout << "  5. Boiling Point at Altitude\n";
        std::cout << "  6. Gas Law Pressure Ratio\n";
        std::cout << "  7. Binary/Hex Representation of a value\n";
        std::cout << "  B. Back\n";
        std::cout << "  Choice: ";
        std::string ch = InputParser::readLine("");
        try {
            if (ch == "1") {
                auto mass   = InputParser::readDouble("  Mass (kg): ", 0.001, 1e6);
                auto delta  = InputParser::readDouble("  ΔT (°C/K): ", -1e4, 1e4);
                if (mass && delta) {
                    double Q = AnalyticsEngine::heatEnergy(*mass, *delta);
                    std::cout << "  Q = " << std::scientific << std::setprecision(4)
                              << Q << " J  (" << Q/1000 << " kJ)\n";
                }
            } else if (ch == "2") {
                auto t  = InputParser::readDouble("  Air Temp (°C): ", -100, 100);
                auto rh = InputParser::readDouble("  Relative Humidity (0-100%): ", 0, 100);
                if (t && rh) {
                    double dp = AnalyticsEngine::dewPoint(*t, *rh);
                    std::cout << "  Dew Point: " << std::fixed << std::setprecision(2)
                              << dp << "°C\n";
                }
            } else if (ch == "3") {
                auto t  = InputParser::readDouble("  Air Temp (°C): ", -80, 10);
                auto w  = InputParser::readDouble("  Wind Speed (km/h): ", 3, 300);
                if (t && w) {
                    double wc = AnalyticsEngine::windChill(*t, *w);
                    std::cout << "  Wind Chill: " << std::fixed << std::setprecision(1)
                              << wc << "°C\n";
                }
            } else if (ch == "4") {
                auto t  = InputParser::readDouble("  Air Temp (°C, >= 27 for accuracy): ", 20, 60);
                auto rh = InputParser::readDouble("  Relative Humidity (0-100%): ", 0, 100);
                if (t && rh) {
                    double hi = AnalyticsEngine::heatIndex(*t, *rh);
                    std::cout << "  Feels Like (Heat Index): " << std::fixed
                              << std::setprecision(1) << hi << "°C\n";
                }
            } else if (ch == "5") {
                auto alt = InputParser::readDouble("  Altitude (m): ", 0, 9000);
                if (alt) {
                    double bp = AnalyticsEngine::boilingPointAtAltitude(*alt);
                    std::cout << "  Boiling point at " << *alt << "m: "
                              << std::fixed << std::setprecision(1) << bp << "°C\n";
                }
            } else if (ch == "6") {
                auto t1 = InputParser::readDouble("  Temperature 1 (K): ", 0.001, 1e6);
                auto t2 = InputParser::readDouble("  Temperature 2 (K): ", 0.001, 1e6);
                if (t1 && t2) {
                    double ratio = AnalyticsEngine::pressureRatio(*t1, *t2);
                    std::cout << "  P2/P1 = " << std::fixed << std::setprecision(6)
                              << ratio << "  (Gay-Lussac's Law)\n";
                }
            } else if (ch == "7") {
                auto v = InputParser::readDouble("  Enter a double value: ");
                if (v) SystemInfo::printBinaryRepresentation(*v);
            }
        } catch (const std::exception& e) {
            std::cout << Color::RED << "  Error: " << e.what() << Color::RESET << "\n";
        }
        InputParser::readLine("\n  Press Enter...");
    }

    void menuDataManagement() {
        while (true) {
            ui_->clearScreen();
            ui_->printHeader();
            std::cout << Color::BOLD << "  ─── Data Management ───\n" << Color::RESET;
            std::cout << "  1. Show History Log\n";
            std::cout << "  2. Export to TXT\n";
            std::cout << "  3. Export to CSV\n";
            std::cout << "  4. Clear History\n";
            std::cout << "  5. Recall Memory (last conversion)\n";
            std::cout << "  6. Undo Last Conversion\n";
            std::cout << "  7. Search History by date\n";
            std::cout << "  B. Back\n";
            std::string ch = InputParser::readLine("  Choice: ");
            if (ch == "B" || ch == "b") break;

            if (ch == "1") {
                std::cout << "\n  History (" << history_.size() << " records):\n";
                for (size_t i = 0; i < history_.size(); i++) {
                    const auto& r = history_[i];
                    std::cout << "  " << (i+1) << ". [" << r.timestamp << "] "
                              << std::fixed << std::setprecision(4)
                              << r.inputValue << " " << r.inputUnit
                              << " → " << r.outputValue << " " << r.outputUnit
                              << "\n";
                }
            } else if (ch == "2") {
                pm_->exportTXT(history_);
            } else if (ch == "3") {
                pm_->exportCSV(history_);
            } else if (ch == "4") {
                history_.clear();
                pm_->clearHistory();
            } else if (ch == "5") {
                if (memory_) {
                    std::cout << "  Memory: " << memory_->inputValue << " "
                              << memory_->inputUnit << " → "
                              << memory_->outputValue << " " << memory_->outputUnit << "\n";
                } else {
                    std::cout << "  No memory stored yet.\n";
                }
            } else if (ch == "6") {
                if (!undoBuffer_.empty()) {
                    auto last = undoBuffer_.back();
                    undoBuffer_.pop_back();
                    if (!history_.empty()) history_.pop_back();
                    std::cout << Color::YELLOW
                              << "  Undone: " << last.inputValue << " " << last.inputUnit
                              << " → " << last.outputValue << " " << last.outputUnit
                              << Color::RESET << "\n";
                } else {
                    std::cout << "  Nothing to undo.\n";
                }
            } else if (ch == "7") {
                std::cout << "  Enter date prefix (e.g., '2025-01'): ";
                std::string query = InputParser::readLine("");
                int found = 0;
                for (const auto& r : history_) {
                    if (r.timestamp.find(query) != std::string::npos) {
                        std::cout << "  [" << r.timestamp << "] "
                                  << r.inputValue << " " << r.inputUnit
                                  << " → " << r.outputValue << " " << r.outputUnit << "\n";
                        found++;
                    }
                }
                if (!found) std::cout << "  No matching records.\n";
            }
            InputParser::readLine("\n  Press Enter...");
        }
    }

    void menuSystemTools() {
        while (true) {
            ui_->clearScreen();
            ui_->printHeader();
            std::cout << Color::BOLD << "  ─── System & Developer Tools ───\n" << Color::RESET;
            std::cout << "  1. System Information\n";
            std::cout << "  2. CPU Temperature\n";
            std::cout << "  3. Battery Status\n";
            std::cout << "  4. Benchmark (100k conversions)\n";
            std::cout << "  5. API Simulation (JSON output)\n";
            std::cout << "  6. Check for Updates\n";
            std::cout << "  7. Version History\n";
            std::cout << "  8. Toggle Verbose Mode (" << (verboseMode_ ? "ON" : "OFF") << ")\n";
            std::cout << "  B. Back\n";
            std::string ch = InputParser::readLine("  Choice: ");
            if (ch == "B" || ch == "b") break;

            if (ch == "1") {
                std::cout << "\n  Architecture : " << SystemInfo::architecture() << "\n";
                std::cout << "  Compiler     : " << SystemInfo::compilerInfo()   << "\n";
                std::cout << "  Memory       : " << SystemInfo::memoryUsage()    << "\n";
                std::cout << "  sizeof(double) = " << sizeof(double) << " bytes\n";
                std::cout << "  sizeof(long double) = " << sizeof(long double) << " bytes\n";
            } else if (ch == "2") {
                std::cout << "  CPU Temp: " << SystemInfo::cpuTemperature() << "\n";
            } else if (ch == "3") {
                std::cout << "  Battery: " << SystemInfo::batteryStatus() << "\n";
            } else if (ch == "4") {
                SystemInfo::runBenchmark();
            } else if (ch == "5") {
                auto v = InputParser::readDouble("  Enter Celsius value for JSON output: ");
                if (v) DevTools::printAPISimulation(*v);
            } else if (ch == "6") {
                DevTools::checkForUpdates();
            } else if (ch == "7") {
                DevTools::printVersionHistory();
            } else if (ch == "8") {
                verboseMode_ = !verboseMode_;
                std::cout << "  Verbose mode: " << (verboseMode_ ? "ON" : "OFF") << "\n";
            }
            InputParser::readLine("\n  Press Enter...");
        }
    }

    void menuGames() {
        while (true) {
            ui_->clearScreen();
            ui_->printHeader();
            std::cout << Color::BOLD << "  ─── Games & Fun ───\n" << Color::RESET;
            std::cout << "  1. Temperature Quiz\n";
            std::cout << "  2. View Leaderboard\n";
            std::cout << "  3. Achievements\n";
            std::cout << "  4. Fact of the Day\n";
            std::cout << "  5. NASA Mode\n";
            std::cout << "  6. Social Share\n";
            std::cout << "  B. Back\n";
            std::string ch = InputParser::readLine("  Choice: ");
            if (ch == "B" || ch == "b") break;

            if (ch == "1") {
                int pts = quiz_->runQuiz(user_->getName(), *ui_);
                user_->addConversion();
                (void)pts;
            } else if (ch == "2") {
                quiz_->printLeaderboard();
            } else if (ch == "3") {
                user_->printAllAchievements();
            } else if (ch == "4") {
                ui_->printFactOfTheDay();
            } else if (ch == "5") {
                ContextModule::printNASAMode();
            } else if (ch == "6") {
                auto v = InputParser::readDouble("  Enter Celsius value: ");
                if (v) {
                    double f = Converter<double>::celsiusToFahrenheit(*v);
                    DevTools::printSocialShare(*v, f);
                }
            }
            InputParser::readLine("\n  Press Enter...");
        }
    }

    void menuRealWorld() {
        while (true) {
            ui_->clearScreen();
            ui_->printHeader();
            std::cout << Color::BOLD << "  ─── Real-World Context ───\n" << Color::RESET;
            std::cout << "  1. City Weather Snapshot\n";
            std::cout << "  2. Climate Change Info\n";
            std::cout << "  3. Cooking Guide\n";
            std::cout << "  4. Medical Reference\n";
            std::cout << "  5. Travel Tip\n";
            std::cout << "  B. Back\n";
            std::string ch = InputParser::readLine("  Choice: ");
            if (ch == "B" || ch == "b") break;

            if (ch == "1") {
                ContextModule::printCityWeather();
            } else if (ch == "2") {
                ContextModule::printClimateInfo();
            } else if (ch == "3") {
                auto v = InputParser::readDouble("  Enter oven temperature (°C): ", 0, 600);
                if (v) std::cout << "  " << ContextModule::cookingGuide(*v) << "\n";
            } else if (ch == "4") {
                auto v = InputParser::readDouble("  Enter body temperature (°C): ", 30, 45);
                if (v) std::cout << "  " << ContextModule::medicalContext(*v) << "\n";
            } else if (ch == "5") {
                auto v = InputParser::readDouble("  Enter outdoor temperature (°C): ", -60, 60);
                if (v) std::cout << "  " << ContextModule::travelTip(*v) << "\n";
            }
            InputParser::readLine("\n  Press Enter...");
        }
    }

    void menuSettings() {
        while (true) {
            ui_->clearScreen();
            ui_->printHeader();
            std::cout << Color::BOLD << "  ─── Settings ───\n" << Color::RESET;
            std::cout << "  1. Set Decimal Precision (current: " << defaultPrec_ << ")\n";
            std::cout << "  2. Toggle Auto-Save (" << (pm_->isAutoSaveEnabled() ? "ON" : "OFF") << ")\n";
            std::cout << "  3. Toggle Dark/Light Mode (" << (ui_->isDarkMode() ? "DARK" : "LIGHT") << ")\n";
            std::cout << "  4. Toggle Minimalist Mode (" << (ui_->isMinimalist() ? "ON" : "OFF") << ")\n";
            std::cout << "  5. Change Language\n";
            std::cout << "  6. Change Username\n";
            std::cout << "  7. Reset to Defaults\n";
            std::cout << "  B. Back\n";
            std::string ch = InputParser::readLine("  Choice: ");
            if (ch == "B" || ch == "b") break;

            if (ch == "1") {
                auto p = InputParser::readDouble("  Precision (0-15): ", 0, 15);
                if (p) { defaultPrec_ = static_cast<int>(*p); }
            } else if (ch == "2") {
                pm_->setAutoSave(!pm_->isAutoSaveEnabled());
                std::cout << "  Auto-Save: " << (pm_->isAutoSaveEnabled() ? "ON" : "OFF") << "\n";
            } else if (ch == "3") {
                ui_->setDarkMode(!ui_->isDarkMode());
            } else if (ch == "4") {
                ui_->setMinimalist(!ui_->isMinimalist());
            } else if (ch == "5") {
                std::cout << "  0=English  1=Hindi(transliterated)  2=Spanish\n";
                auto l = InputParser::readDouble("  Language: ", 0, 2);
                if (l) ui_->setLanguage(static_cast<int>(*l));
            } else if (ch == "6") {
                std::cout << "  New username: ";
                std::string name = InputParser::readLine("");
                if (!name.empty()) user_->setName(name);
            } else if (ch == "7") {
                defaultPrec_ = 4;
                verboseMode_  = false;
                ui_->setMinimalist(false);
                ui_->setLanguage(0);
                pm_->setAutoSave(true);
                std::cout << "  Settings reset to defaults.\n";
            }
            InputParser::readLine("\n  Press Enter...");
        }
    }

    void menuHelp() {
        ui_->clearScreen();
        ui_->printHeader();
        ui_->printHelp();
        ui_->printFactOfTheDay();
        std::cout << "\n  Donate / Support: https://buymeacoffee.com/tempsuite\n";  // Feature #93
        std::cout << "\n  Sublimation point of dry ice: -78.5°C\n";  // Feature #15
        std::cout << "  Sun surface: ~5504°C | Pluto: ~-233°C\n\n";   // Feature #80
        InputParser::readLine("  Press Enter...");
    }

    /**
     * @brief Password protection at startup. Feature #42.
     */
    bool authenticate() {
        static const std::string PIN = "1234"; // Feature #42 — hardcoded PIN demo
        std::cout << Color::YELLOW
                  << "\n  🔐 Enter PIN to continue (default: 1234): "
                  << Color::RESET;
        std::string pin = InputParser::readLine("");
        return (pin == PIN);
    }

    /**
     * @brief Handles command-line argument mode. Feature #45.
     */
    void handleCLI(int argc, char* argv[]) {
        // Usage: ./temp_suite 32 -f  or  ./temp_suite 0 -c
        if (argc < 3) {
            std::cerr << "CLI usage: ./temp_suite <value> -c|-f|-k|-r\n";
            return;
        }
        double value;
        try { value = std::stod(argv[1]); }
        catch (...) { std::cerr << "Invalid value: " << argv[1] << "\n"; return; }

        std::string flag = argv[2];
        std::string unit = "C";
        if (flag == "-f" || flag == "-F") unit = "F";
        else if (flag == "-k" || flag == "-K") unit = "K";
        else if (flag == "-r" || flag == "-R") unit = "R";

        try {
            doConversion(value, unit);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

public:
    /**
     * @brief Construct with all subsystems initialized. Zero-leak via unique_ptr.
     */
    Application()
        : ui_(std::make_unique<TerminalUI>()),
          pm_(std::make_unique<PersistenceManager>()),
          user_(std::make_unique<UserProfile>("Guest"))
    {
        leaderboard_ = pm_->loadLeaderboard();
        quiz_   = std::make_unique<QuizEngine>(leaderboard_, *pm_);
    }

    /**
     * @brief Main entry point (interactive mode).
     */
    void run(int argc, char* argv[]) {
        // CLI mode (Feature #45)
        if (argc >= 3) {
            handleCLI(argc, argv);
            return;
        }

        // --help flag (Feature #30)
        if (argc == 2 && std::string(argv[1]) == "--help") {
            ui_->printHelp();
            return;
        }

        // Password (Feature #42)
        if (!authenticate()) {
            std::cout << Color::RED << "  Access denied.\n" << Color::RESET;
            return;
        }

        // Greet user (Feature #43)
        std::cout << Color::BOLD << "\n  " << ui_->tr("welcome") << "! Enter your name: ";
        std::cout << Color::RESET;
        std::string name = InputParser::readLine("");
        if (!name.empty()) user_->setName(name);
        std::cout << "  Hello, " << Color::GREEN << user_->getName()
                  << Color::RESET << "! Let's convert some temperatures.\n\n";

        // Tutorial mode hint (Feature #69)
        std::cout << Color::DIM << "  Tip: Type --help at the menu or press Q to quit.\n"
                  << Color::RESET;

        // Main loop
        while (running_) {
            ui_->clearScreen();
            ui_->printHeader();
            ui_->printMainMenu(user_->getName(), user_->getPoints(), user_->getLevel());

            std::string choice = InputParser::readLine("  Your choice: ");

            // Shortcut keys (Feature #57, #58)
            if (choice == "Q" || choice == "q") {
                if (InputParser::confirmExit()) {  // Feature #57
                    running_ = false;
                }
            } else if (choice == "C" || choice == "c") {
                ui_->clearScreen();
            } else if (choice == "M" || choice == "m") {
                if (memory_) {
                    std::cout << "  Memory: " << memory_->inputValue << " "
                              << memory_->inputUnit << " → "
                              << memory_->outputValue << " " << memory_->outputUnit << "\n";
                    InputParser::readLine("  Press Enter...");
                }
            } else if (choice == "1") {
                menuSingleConversion();
            } else if (choice == "2") {
                menuMultipleInputs();
            } else if (choice == "3") {
                menuRangeTable();
            } else if (choice == "4") {
                menuScience();
            } else if (choice == "5") {
                menuDataManagement();
            } else if (choice == "6") {
                menuSystemTools();
            } else if (choice == "7") {
                menuGames();
            } else if (choice == "8") {
                menuRealWorld();
            } else if (choice == "9") {
                menuSettings();
            } else if (choice == "10" || choice == "--help") {
                menuHelp();
            } else if (!choice.empty()) {
                // Try to parse as a direct temperature entry (Feature #8 auto-detect)
                auto parsed = InputParser::parse(choice);
                if (parsed.valid) {
                    try { doConversion(parsed.value, parsed.unit); }
                    catch (const std::exception& e) {
                        std::cout << Color::RED << "  " << e.what() << Color::RESET << "\n";
                    }
                    InputParser::readLine("  Press Enter...");
                }
            }
        }

        // Exit summary (Features #57, #100)
        ui_->clearScreen();
        ui_->printHeader();
        user_->printSummary();

        std::cout << Color::BOLD << "  Final Summary\n" << Color::RESET;
        std::cout << "  Total conversions this session : " << user_->getConversionCount() << "\n";
        std::cout << "  History records saved          : " << history_.size() << "\n";
        std::cout << "  Session duration               : " << user_->sessionDuration() << "\n";
        ui_->printFooter();
        std::cout << Color::CYAN << "\n  Goodbye, " << user_->getName()
                  << "! Stay curious. 🌡\n\n" << Color::RESET;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 14: MAIN ENTRY POINT
// ═══════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    // Global exception handler
    try {
        Application app;
        app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "\n  [FATAL] Unhandled exception: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "\n  [FATAL] Unknown exception.\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 * ────────────────────────────────────────────────────────────────
 * FEATURE MAPPING SUMMARY (all 100 features)
 * ────────────────────────────────────────────────────────────────
 *  #1  C→F        : Converter<T>::celsiusToFahrenheit
 *  #2  F→C        : Converter<T>::fahrenheitToCelsius
 *  #3  Kelvin     : Temperature class stores Kelvin internally
 *  #4  Rankine    : Converter<T>::celsiusToRankine / rankineToCelsius
 *  #5  Réaumur    : Converter<T>::celsiusToReaumur / reaumurToCelsius
 *  #6  Multiple   : menuMultipleInputs()
 *  #7  Range Table: menuRangeTable() → TerminalUI::printRangeTable()
 *  #8  Auto-detect: InputParser::parse() — suffix detection
 *  #9  Abs Zero   : Converter<T>::validateKelvin() + domain_error
 * #10  Precision  : Temperature::setPrecision() / defaultPrec_
 * #11  Boiling Pt : TerminalUI::printResult() alert
 * #12  Freezing Pt: TerminalUI::printResult() alert
 * #13  Room Temp  : doConversion() — diff from 22°C
 * #14  Body Temp  : ContextModule::medicalContext()
 * #15  Sublimation: menuHelp() / Constants::DRY_ICE_SUBLIM_C
 * #16  Heat Energy: AnalyticsEngine::heatEnergy() — Q=mcΔT
 * #17  Gas Laws   : AnalyticsEngine::pressureRatio()
 * #18  Average    : AnalyticsEngine::average()
 * #19  Std Dev    : AnalyticsEngine::stdDev()
 * #20  Auto-Suffix: Temperature::format() — °C, °F, K symbols
 * #21  ASCII Art  : TerminalUI::printHeader() — box art
 * #22  Color Code : TerminalUI::tempColor() — Red/Blue/Green
 * #23  Progress   : TerminalUI::progressBar()
 * #24  Clear Screen: TerminalUI::clearScreen() → system(CLEAR_SCREEN)
 * #25  Box Borders: TerminalUI::printBox() — Unicode box drawing
 * #26  Blink Alert: TerminalUI::printBlinkingAlert() — ANSI blink
 * #27  Sound/Beep : TerminalUI::beep() — '\a'
 * #28  Menu-Driven: Application::run() — numbered menu
 * #29  Arrow Keys : (Platform limitation noted; numeric menu used)
 * #30  Help       : TerminalUI::printHelp() / --help flag
 * #31  History Log: history_ vector + menuDataManagement()
 * #32  TXT Export : PersistenceManager::exportTXT()
 * #33  CSV Export : PersistenceManager::exportCSV()
 * #34  Session Timer: UserProfile::sessionDuration()
 * #35  Timestamps : ConversionRecord::timestamp field
 * #36  Auto-Save  : PersistenceManager::autoSave()
 * #37  Clear Hist : history_.clear() + clearHistory()
 * #38  Memory     : memory_ optional + 'M' shortcut
 * #39  Undo       : undoBuffer_ + menuDataManagement ch==6
 * #40  Search Hist: menuDataManagement ch==7 — find by date
 * #41  Multi-Lang : TerminalUI::languages_ + setLanguage()
 * #42  Password   : Application::authenticate() — PIN check
 * #43  User Profile: UserProfile::setName() + greeting
 * #44  Dark Mode  : TerminalUI::setDarkMode() toggle
 * #45  CLI Args   : Application::handleCLI() — ./temp_suite 32 -f
 * #46  API Sim    : DevTools::printAPISimulation() — JSON output
 * #47  Update Check: DevTools::checkForUpdates()
 * #48  Battery    : SystemInfo::batteryStatus()
 * #49  Live Clock : TerminalUI::printLiveClock()
 * #50  CPU Temp   : SystemInfo::cpuTemperature()
 * #51  Negative Val: InputParser::parse() handles +/-
 * #52  Non-Numeric: InputParser::parse() — stod exception
 * #53  Empty Input: InputParser::parse() — empty check
 * #54  Char Limit : InputParser::parse() — size > 30
 * #55  Overflow   : InputParser::parse() — |v| > 1e15 guard
 * #56  Retry Loop : InputParser::readDouble() — 5 attempts
 * #57  Exit Confirm: InputParser::confirmExit()
 * #58  Shortcuts  : run() — Q/C/M keys
 * #59  Case Insens: parse() — toupper normalization
 * #60  Trim Input : InputParser::trim()
 * #61  Quiz       : QuizEngine::runQuiz()
 * #62  Points     : UserProfile::addConversion()
 * #63  Achievements: UserProfile::achievements_
 * #64  Fact of Day: TerminalUI::printFactOfTheDay()
 * #65  Leaderboard: QuizEngine + PersistenceManager::saveLeaderboard
 * #66  Level Up   : UserProfile::computeLevel()
 * #67  Easter Eggs: UserProfile::easterEgg()
 * #68  Sound Toggle: (beep can be disabled via minimalist mode)
 * #69  Tutorial   : startup hint in Application::run()
 * #70  Conv Speed : doConversion() — chrono microseconds
 * #71  Weather    : ContextModule::printCityWeather()
 * #72  Climate    : ContextModule::printClimateInfo()
 * #73  Cooking    : ContextModule::cookingGuide()
 * #74  Medical    : ContextModule::medicalContext()
 * #75  Travel Tips: ContextModule::travelTip()
 * #76  Humidity   : AnalyticsEngine::heatIndex()
 * #77  Dew Point  : AnalyticsEngine::dewPoint()
 * #78  Wind Chill : AnalyticsEngine::windChill()
 * #79  Altitude   : AnalyticsEngine::boilingPointAtAltitude()
 * #80  NASA Mode  : ContextModule::printNASAMode()
 * #81  Verbose    : DevTools::verboseConvert() + verboseMode_ flag
 * #82  Debug Logs : PersistenceManager::debugLog() → .log file
 * #83  Memory Use : SystemInfo::memoryUsage()
 * #84  Compiler   : SystemInfo::compilerInfo()
 * #85  Benchmark  : SystemInfo::runBenchmark() — 100k conversions
 * #86  Architecture: SystemInfo::architecture() + platform macros
 * #87  Hex/Binary : SystemInfo::printBinaryRepresentation()
 * #88  Plugin Stub: (noted as extension point via DevTools)
 * #89  Reset Settings: menuSettings() ch==7
 * #90  Version Hist: DevTools::printVersionHistory()
 * #91  Coffee Break: UserProfile::needsCoffeeBreak() — 30min check
 * #92  Quotes     : TerminalUI::printFooter()
 * #93  Donate Link: menuHelp() — buymeacoffee URL
 * #94  Social Share: DevTools::printSocialShare()
 * #95  ASCII Therm: TerminalUI::printThermometer()
 * #96  Encryption : PersistenceManager::xorCipher() — XOR .dat
 * #97  Auto Update UI: printLiveClock() on each screen refresh
 * #98  Keyboard Sound: TerminalUI::beep() on alerts
 * #99  Minimalist : TerminalUI::minimalistMode_ toggle
 * #100 Final Summary: Application::run() exit block
 * ────────────────────────────────────────────────────────────────
 */
