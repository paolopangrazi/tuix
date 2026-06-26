#include "formula_catalog.hpp"

#include <cctype>

const FormulaInfo k_formulas[] = {
    { "ABS",         "ABS(number)",                "Absolute value (removes sign)"          },
    { "AVERAGE",     "AVERAGE(range)",             "Arithmetic mean of numeric cells"       },
    { "AVERAGEIF",   "AVERAGEIF(range, crit, [avg])", "Mean of cells meeting a criterion"   },
    { "CONCATENATE", "CONCATENATE(text1, text2…)", "Join two or more text strings"          },
    { "COUNT",       "COUNT(range)",               "Count cells that contain numbers"       },
    { "COUNTA",      "COUNTA(range)",              "Count non-empty cells"                  },
    { "COUNTIF",     "COUNTIF(range, criterion)",  "Count cells meeting a criterion"        },
    { "IF",          "IF(cond, true, false)",       "Return one of two values based on test" },
    { "IFERROR",     "IFERROR(expr, fallback)",     "Return fallback if expr errors"         },
    { "IFNA",        "IFNA(expr, fallback)",        "Return fallback if expr is #N/A"        },
    { "IFS",         "IFS(cond1, val1, …)",         "First value whose condition is true"    },
    { "INDEX",       "INDEX(range, row, [col])",   "Cell at a position within a range"      },
    { "INT",         "INT(number)",                "Truncate to integer toward zero"        },
    { "LEN",         "LEN(text)",                  "Number of characters in text"           },
    { "LOWER",       "LOWER(text)",                "Convert text to lower case"             },
    { "MATCH",       "MATCH(key, range, [type])",  "Position of key within a 1-D range"     },
    { "MAX",         "MAX(range)",                 "Largest numeric value in range"         },
    { "MIN",         "MIN(range)",                 "Smallest numeric value in range"        },
    { "MOD",         "MOD(number, divisor)",        "Remainder after division"               },
    { "ROUND",       "ROUND(number, digits)",       "Round to given decimal places"          },
    { "SQRT",        "SQRT(number)",               "Square root"                            },
    { "SUM",         "SUM(range)",                 "Sum all numeric values in range"        },
    { "SUMIF",       "SUMIF(range, crit, [sum])",  "Sum cells meeting a criterion"          },
    { "TRIM",        "TRIM(text)",                 "Remove leading/trailing whitespace"     },
    { "UPPER",       "UPPER(text)",                "Convert text to upper case"             },
    { "VLOOKUP",     "VLOOKUP(key, range, col, [exact])", "Look up key in range's first column" },
};
const int k_formulas_count = (int)(sizeof(k_formulas) / sizeof(k_formulas[0]));

std::string ac_prefix(const std::string& buf) {
    int i = (int)buf.size();
    while (i > 0 && std::isalpha((unsigned char)buf[i - 1])) --i;
    return buf.substr(i);
}

bool should_show_ac(const std::string& buf) {
    if (buf.empty() || buf[0] != '=') return false;
    return buf.size() == 1 || std::isalpha((unsigned char)buf.back());
}
