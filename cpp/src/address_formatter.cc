// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <libaddressinput/address_formatter.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/util/basictypes.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "format_element.h"
#include "language.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/cctype_tolower_equal.h"

namespace i18n {
namespace addressinput {

namespace {

const char kCommaSeparator[] = ", ";
const char kSpaceSeparator[] = " ";
const char kArabicCommaSeparator[] = "\xD8\x8C" " ";  /* "، " */

const char* kLanguagesThatUseSpace[] = {
  "th",
  "ko"
};

const char* kLanguagesThatHaveNoSeparator[] = {
  "ja",
  "zh"  // All Chinese variants.
};

// This data is based on CLDR, for languages that are in official use in some
// country, where Arabic is the most likely script tag.
// TODO: Consider supporting variants such as tr-Arab by detecting the script
// code.
const char* kLanguagesThatUseAnArabicComma[] = {
  "ar",
  "az",
  "fa",
  "kk",
  "ku",
  "ky",
  "ps",
  "tg",
  "tk",
  "ur",
  "uz"
};

std::string GetLineSeparatorForLanguage(const std::string& language_tag) {
  Language address_language(language_tag);

  // First deal with explicit script tags.
  if (address_language.has_latin_script) {
    return kCommaSeparator;
  }

  // Now guess something appropriate based on the base language.
  const std::string& base_language = address_language.base;
  if (std::find_if(kLanguagesThatUseSpace,
                   kLanguagesThatUseSpace + arraysize(kLanguagesThatUseSpace),
                   std::bind2nd(EqualToTolowerString(), base_language)) !=
      kLanguagesThatUseSpace + arraysize(kLanguagesThatUseSpace)) {
    return kSpaceSeparator;
  } else if (std::find_if(
                 kLanguagesThatHaveNoSeparator,
                 kLanguagesThatHaveNoSeparator +
                     arraysize(kLanguagesThatHaveNoSeparator),
                 std::bind2nd(EqualToTolowerString(), base_language)) !=
             kLanguagesThatHaveNoSeparator +
                 arraysize(kLanguagesThatHaveNoSeparator)) {
    return "";
  } else if (std::find_if(
                 kLanguagesThatUseAnArabicComma,
                 kLanguagesThatUseAnArabicComma +
                     arraysize(kLanguagesThatUseAnArabicComma),
                 std::bind2nd(EqualToTolowerString(), base_language)) !=
             kLanguagesThatUseAnArabicComma +
                 arraysize(kLanguagesThatUseAnArabicComma)) {
    return kArabicCommaSeparator;
  }
  // Either the language is a latin-script language, or no language was
  // specified. In the latter case we still return ", " as the most common
  // separator in use. In countries that don't use this, e.g. Thailand,
  // addresses are often written in latin script where this would still be
  // appropriate, so this is a reasonable default in the absence of information.
  return kCommaSeparator;
}

void CombineLinesForLanguage(const std::vector<std::string>& lines,
                             const std::string& language_tag,
                             std::string* line) {
  line->clear();
  std::string separator = GetLineSeparatorForLanguage(language_tag);
  for (std::vector<std::string>::const_iterator it = lines.begin();
       it != lines.end();
       ++it) {
    if (it != lines.begin()) {
      line->append(separator);
    }
    line->append(*it);
  }
}

}  // namespace

void GetFormattedNationalAddress(
    const AddressData& address_data, std::vector<std::string>* lines) {
  assert(lines != NULL);
  lines->clear();

  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  // TODO: Eventually, we should get the best rule for this country and
  // language, rather than just for the country.
  rule.ParseSerializedRule(RegionDataConstants::GetRegionData(
      address_data.region_code));

  Language language(address_data.language_code);

  // If latinized rules are available and the |language_code| of this address is
  // explicitly tagged as being Latin, then use the latinized formatting rules.
  const std::vector<FormatElement>& format =
      language.has_latin_script && !rule.GetLatinFormat().empty()
          ? rule.GetLatinFormat()
          : rule.GetFormat();

  std::string line;
  for (size_t i = 0; i < format.size(); ++i) {
    FormatElement element = format[i];
    if (element.IsNewline()) {
      if (!line.empty()) {
        lines->push_back(line);
        line.clear();
      }
    } else if (element.IsField()) {
      AddressField field = element.GetField();
      if (field == STREET_ADDRESS) {
        // The field "street address" represents the street address lines of an
        // address, so there can be multiple values.
        if (!address_data.IsFieldEmpty(field)) {
          line.append(address_data.address_line.front());
          if (address_data.address_line.size() > 1U) {
            lines->push_back(line);
            line.clear();
            lines->insert(lines->end(),
                          address_data.address_line.begin() + 1,
                          address_data.address_line.end());
          }
        }
      } else {
        line.append(address_data.GetFieldValue(field));
      }
    } else {
      line.append(element.GetLiteral());
    }
  }
  if (!line.empty()) {
    lines->push_back(line);
  }
}

void GetFormattedNationalAddressLine(
    const AddressData& address_data, std::string* line) {
  std::vector<std::string> address_lines;
  GetFormattedNationalAddress(address_data, &address_lines);
  CombineLinesForLanguage(address_lines, address_data.language_code, line);
}

void GetStreetAddressLinesAsSingleLine(
    const AddressData& address_data, std::string* line) {
  CombineLinesForLanguage(
      address_data.address_line, address_data.language_code, line);
}

}  // namespace addressinput
}  // namespace i18n
