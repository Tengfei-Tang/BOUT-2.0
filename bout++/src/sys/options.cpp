/**************************************************************************
 * Reads in the configuration file, supplying
 * an interface to get options
 * 
 * File is an ini file with sections
 * [section]
 * and variables as
 * name = string ; comment
 *
 * ChangeLog
 * =========
 * 
 * 2010-06 Sean Farley 
 *    * Re-written to use STL maps
 *
 * 2010-02-10 Ben Dudson <bd512@york.ac.uk>
 *    
 *    * Adding set methods to allow other means to control code
 *      Intended to help with integration into FACETS
 *
 * 2007-09-01 Ben Dudson <bd512@york.ac.uk>
 *
 *    * Initial version
 *
 **************************************************************************
 * Copyright 2010 B.D.Dudson, S.Farley, M.V.Umansky, X.Q.Xu
 *
 * Contact: Ben Dudson, bd512@york.ac.uk
 * 
 * This file is part of BOUT++.
 *
 * BOUT++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BOUT++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with BOUT++.  If not, see <http://www.gnu.org/licenses/>.
 * 
 **************************************************************************/

#include "globals.h"
#include "options.h"
#include "utils.h"
#include "boutexception.h"

#include <typeinfo>
#include <sstream>

OptionFile::OptionFile() : sep("_")
{
}

OptionFile::OptionFile(const string &filename) : sep("_")
{
  read(filename.c_str());
}

OptionFile::OptionFile(int &argc, char **argv, const string &filename) : sep("_")
{
  read(filename.c_str());
  commandLineRead(argc, argv);
}

OptionFile::~OptionFile()
{
}


/**************************************************************************
 * Read input file
 **************************************************************************/

void OptionFile::read(const char *format, ...)
{
  ifstream fin;

  string buffer;
  string section; // Current section
  
  size_t startpos, endpos;

  va_list ap;  // List of arguments
  char filename[512];
  
  if(format == (const char*) NULL) {
    throw BoutException("ERROR: OptionFile::read passed NULL filename\n");
  }

  va_start(ap, format);
  vsprintf(filename, format, ap);
  va_end(ap);

  output.write("Reading options file %s\n", filename);
  
  fin.open(filename);

  if(!fin.good()) {
    throw BoutException("\tOptions file '%s' not found\n", filename);
  }

  do {
    buffer = getNextLine(fin);
    
    if(!buffer.empty()) {

      // Check for section
      startpos = buffer.find_first_of("[");
      endpos   = buffer.find_last_of("]");

      if( startpos != string::npos ) {
        if( endpos == string::npos ) {
          throw BoutException("\t'%s': Missing ']'\n\tLine: %s", filename, buffer.c_str());
        }

        trim(buffer, "[]");

        if(!buffer.empty()) {
          section = buffer;
        }
      } else {
        
        string key, value;
        
        parse(buffer, key, value);

        add(section, key, value);
      } // section test
    } // buffer.empty
  } while(!fin.eof());

  fin.close();
  
  if(options.empty()) {
    throw BoutException("\tEmpty option file '%s'\n", filename);
  }
}

void OptionFile::commandLineRead(int argc, char** argv)
{
  output << "Checking command-line options\n";

  string buffer;
  string key, value;

  // Go through command-line arguments
  for(size_t i=1;i<argc;i++) {
    // Should contain a "key=value" string
    buffer = argv[i];
    
    parse(buffer, key, value);
    add("", key, value, "command line");
  }
}

/**************************************************************************
 * Functions related to sections
 **************************************************************************/

void OptionFile::setSection(const string &name) // Set the default section
{
  def_section = name;
}

string OptionFile::getSection() // Set the default section
{
  return def_section;
}

void OptionFile::setSectionSep(const string &s)
{
  sep = s;
}

inline const string& OptionFile::getSectionSep()
{
  return sep; ///< Used to separate sections and keys
}

inline string OptionFile::prependSection(const string &section, const string& key)
{
  if(!section.empty())
    return lowercase(section + getSectionSep() + key);
    
  return lowercase(key);
}

/**************************************************************************
 * Test if options have been set
 **************************************************************************/

bool OptionFile::isSet(const string &key)
{
  if(find(key) != end())
    return true;
  
  if(find(prependSection(def_section, key)) != end())
    return true;
  
  return false;
}

bool OptionFile::isSet(const string &section, const string &key)
{
  if(find(prependSection(def_section, key)) != end())
    return true;
  
  return false;
}

/**************************************************************************
 * Functions to request options
 **************************************************************************/

template <class type>
void OptionFile::get(const map<string,Option>::iterator &it, type &val)
{
  if(it != end()) {
    
    stringstream ss;

    ss << it->second.value;
    ss >> val;
    output << "\tOption " << it->first << " = " << val;
    
    if(!it->second.source.empty()) {
      // Specify the source of the setting
      output << " (" << it->second.source << ")";
    }
    
    output << endl;
  }
}

template void OptionFile::get<int>(const map<string,Option>::iterator &it, int &val);
template void OptionFile::get<BoutReal>(const map<string,Option>::iterator &it, BoutReal &val);

/// Can't use stringstream as it breaks on whitespace
void OptionFile::get(const map<string,Option>::iterator &it, string &val)
{
  if(it != end()) {
    
    val = it->second.value;
    
    output << "\tOption " << it->first << " = " << val;
    
    if(!it->second.source.empty()) {
      // Specify the source of the setting
      output << " (" << it->second.source << ")";
    }
    
    output << endl;
  }
}

void OptionFile::get(const map<string,Option>::iterator &it, bool &val)
{
  if(it != end()) {
    
    char c = toupper((it->second.value)[0]);
    if((c == 'Y') || (c == 'T') || (c == '1')) {
      val = true;
      output << "\tOption " << it->first << " = true";
    } else if((c == 'N') || (c == 'F') || (c == '0')) {
      val = false;
      output << "\tOption " << it->first << " = false";
    } else
      throw BoutException("\tOption '%s': Boolean expected. Got '%s'\n", 
                          it->first.c_str(), it->second.value.c_str());
    if(!it->second.source.empty()) {
      // Specify the source of the setting
      output << " (" << it->second.source << ")";
    }
    
    output << endl;
  }
}

////////////////////////////////////////////////////////////////////

template<class type>
void OptionFile::get(const string &key, type &val, const type &def)
{
  map<string, Option>::iterator it(find(key));

  if(it != end()) {
    get<type>(it, val);
    return;
  }
  
  it = find(prependSection(def_section, key));
  if(it != end()) {
    get<type>(it, val);
    return;
  }

  val = def;
  output << "\tOption " << key << " = " << def << " (default)" << endl;
}

template void OptionFile::get<int>(const string &key, int &val, const int &def);
template void OptionFile::get<BoutReal>(const string &key, BoutReal &val, const BoutReal &def);

void OptionFile::get(const string &key, string &val, const string &def)
{
  map<string, Option>::iterator it(find(key));

  if(it != end()) {
    get(it, val);
    return;
  }
  
  it = find(prependSection(def_section, key));
  if(it != end()) {
    get(it, val);
    return;
  }

  val = def;
  output << "\tOption " << key << " = " << def << " (default)" << endl;
}

void OptionFile::get(const string &key, bool &val, const bool &def)
{
  map<string, Option>::iterator it(find(key));

  if(it != end()) {
    get(it, val);
    return;
  }
  
  it = find(prependSection(def_section, key));
  if(it != end()) {
    get(it, val);
    return;
  }

  val = def;
  if(def) {
    output << "\tOption " << key << " = true (default)" << endl;
  }else
    output << "\tOption " << key << " = false (default)" << endl;
}

////////////////////////////////////////////////////////////////////

template<class type>
void OptionFile::get(const string &section, const string &key, type &val, const type &def)
{
  if(key.empty()) {
    output.write("WARNING: NULL option requested\n");
    return;
  }
  
  get<type>(prependSection(section, key), val, def);
}

template void OptionFile::get<int>(const string &section, const string &key, int &val, const int &def);
template void OptionFile::get<BoutReal>(const string &section, const string &key, BoutReal &val, const BoutReal &def);

void OptionFile::get(const string &section, const string &key, string &val, const string &def)
{
  if(key.empty()) {
    output.write("WARNING: NULL option requested\n");
    return;
  }
  
  get(prependSection(section, key), val, def);
}

void OptionFile::get(const string &section, const string &key, bool &val, const bool &def)
{
  if(key.empty()) {
    output.write("WARNING: NULL option requested\n");
    return;
  }
  
  get(prependSection(section, key), val, def);
}

///////////////////////////////////////////////////////////////////////////////

template<class type>
void OptionFile::get(const string &section1, const string &section2, const string &key, type &val, const type &def)
{
  if(isSet(prependSection(section1, key))) {
    get<type>(prependSection(section1, key), val, def);
    return;
  }
  
  get<type>(prependSection(section2, key), val, def);
}

template void OptionFile::get<int>(const string &section1, const string &section2, const string &key, int &val, const int &def);
template void OptionFile::get<BoutReal>(const string &section1, const string &section2, const string &key, BoutReal &val, const BoutReal &def);

void OptionFile::get(const string &section1, const string &section2, const string &key, string &val, const string &def)
{
  if(isSet(prependSection(section1, key))) {
    get(prependSection(section1, key), val, def);
    return;
  }
  
  get(prependSection(section2, key), val, def);
}

void OptionFile::get(const string &section1, const string &section2, const string &key, bool &val, const bool &def)
{
  if(isSet(prependSection(section1, key))) {
    get(prependSection(section1, key), val, def);
    return;
  }
  
  get(prependSection(section2, key), val, def);
}

///////////////////////////////////////////////////////////////////////////////

void OptionFile::get(const string &key, int &val, const int &def)
{
  get<int>(key, val, def);
}

void OptionFile::get(const string &key, BoutReal &val, const BoutReal &def)
{
  get<BoutReal>(key, val, def);
}

void OptionFile::get(const string &section, const string &key, int &val, const int &def)
{
  get<int>(section, key, val, def);
}

void OptionFile::get(const string &section1, const string &section2, const string &key, int &val, const int &def)
{
  get<int>(section1, section2, key, val, def);
}

void OptionFile::get(const string &section, const string &key, BoutReal &val, const BoutReal &def)
{
  get<BoutReal>(section, key, val, def);
}

void OptionFile::get(const string &section1, const string &section2, const string &key, BoutReal &val, const BoutReal &def)
{
  get<BoutReal>(section1, section2, key, val, def);
}

/**************************************************************************
 * Set methods
 *
 * Convert given options to strings
 **************************************************************************/

template<class type>
void OptionFile::set(const string &key, const type &val)
{
  stringstream ss;
  
  ss << val;
  
  add("", key, ss.str(), "set");
}

template void OptionFile::set<int>(const string &key, const int &val);
template void OptionFile::set<BoutReal>(const string &key, const BoutReal &val);
template void OptionFile::set<bool>(const string &key, const bool &val);
template void OptionFile::set<string>(const string &key, const string &val);

void OptionFile::set(const string &key, const int &val)
{
  set<int>(key, val);
}

void OptionFile::set(const string &key, const BoutReal &val)
{
  set<BoutReal>(key, val);
}

void OptionFile::set(const string &key, const bool &val)
{
  set<bool>(key, val);
}

void OptionFile::set(const string &key, const string &val)
{
  set<string>(key, val);
}

/**************************************************************************
 * Print unused options
 **************************************************************************/

void OptionFile::printUnused()
{
  bool allused = true;
  // Check if any options are unused
  for(map<string,Option>::iterator it=options.begin(); it != options.end(); it++) {
    if(!it->second.used) {
      allused = false;
      break;
    }
  }
  if(allused) {
    output << "All options used\n";
  }else {
    output << "Unused options:\n";
    for(map<string,Option>::iterator it=options.begin(); it != options.end(); it++) {
      if(!it->second.used) {
	output << "\t" << it->first << " = " << it->second.value;
	if(!it->second.source.empty())
	  output << " (" << it->second.source << ")";
	output << endl;
      }
    }
  }
  
}

/**************************************************************************
 * Private functions
 **************************************************************************/

void OptionFile::add(const string &section, const string &key, const string &value, const string &source)
{
  if(key.empty()) {
    throw BoutException("\tEmpty key passed to 'OptionsFile::add!'");
  }

  string sectionkey(prependSection(section, key));

  Option opt;
  opt.value = value;
  opt.source = source;
  opt.used = false;

  options[sectionkey] = opt;
}

map<string,Option>::iterator OptionFile::find(const string &key)
{
  map<string,Option>::iterator it(options.find(key));
  if(it != options.end())
    it->second.used = true;
  return it;
}

map<string,Option>::iterator OptionFile::find(const string &section, const string &key)
{
  return find(prependSection(section, key));
}

map<string,Option>::iterator OptionFile::end()
{
  return options.end();
}

// Strips leading and trailing spaces from a string
void OptionFile::trim(string &s, const string &c)
{
  
  // Find the first character position after excluding leading blank spaces
  size_t startpos = s.find_first_not_of(c);
  // Find the first character position from reverse af
  size_t endpos = s.find_last_not_of(c);

  // if all spaces or empty, then return an empty string
  if(( startpos == string::npos ) || ( endpos == string::npos ))
  {
    s = "";
  }
  else
  {
    s = s.substr(startpos, endpos-startpos+1);
  }
}

void OptionFile::trimRight(string &s, const string &c)
{  
  // Find the first character position after excluding leading blank spaces
  size_t startpos = s.find_first_of(c);

  // if all spaces or empty, then return an empty string
  if(( startpos != string::npos ) )
  {
    s = s.substr(0,startpos);
  }
}

void OptionFile::trimLeft(string &s, const string &c)
{
  
  // Find the first character position from reverse af
  size_t endpos = s.find_last_of(c);

  // if all spaces or empty, then return an empty string
  if(( endpos != string::npos ) )
  {  
    s = s.substr(endpos);
  }
}

// Strips the comments from a string
void OptionFile::trimComments(string &s)
{
  trimRight(s, "#;");
}

// Returns the next useful line, stripped of comments and whitespace
string OptionFile::getNextLine(ifstream &fin)
{
  string line("");
  
  getline(fin, line);
  trimComments(line);
  trim(line);
  line = lowercasequote(line); // lowercase except for inside quotes
  
  return line;
}

void OptionFile::parse(const string &buffer, string &key, string &value)
{
   // A key/value pair, separated by a '='

  size_t startpos = buffer.find_first_of("=");
  size_t endpos   = buffer.find_last_of("=");

  if(startpos == string::npos) {
    // Just set a flag to true
    // e.g. "restart" or "append" on command line
    key = buffer;
    value = string("TRUE");
    return;
  }

  key = buffer.substr(0, startpos);
  value = buffer.substr(startpos+1);

  trim(key, " \t\"");
  trim(value, " \t\"");

  if(key.empty() || value.empty()) {
    throw BoutException("\tEmpty key or value\n\tLine: %s", buffer.c_str());
  }
}
