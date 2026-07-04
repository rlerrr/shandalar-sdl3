#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/assert.h"

#ifndef _MSC_VER
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

// FUNCTION: SHANDALAR 0x004c8381
int IniHexDigitToInt(int ch)
{
  if (ch >= 'a' && 'f' >= ch)
  {
    return ch - 'W';
  }
  else
  {
    return ch - '0';
  }
}

// FUNCTION: SHANDALAR 0x004c8587
int TrimIniTrailingWhitespace(char *line, int line_len)
{
  int trimmed_count;

  trimmed_count = 0;
  if (line_len == 0)
  {
    return 0;
  }

  while (trimmed_count < line_len &&
         (line[(line_len - 1) - trimmed_count] == ' ' ||
          line[(line_len - 1) - trimmed_count] == '\t' ||
          line[(line_len - 1) - trimmed_count] == '\r' ||
          line[(line_len - 1) - trimmed_count] == '\n'))
  {
    trimmed_count = trimmed_count + 1;
  }

  line[line_len - trimmed_count] = '\0';
  return trimmed_count;
}

// FUNCTION: SHANDALAR 0x004c84c7
size_t StripIniCommentSuffix(char *line, size_t line_len)
{
  size_t removed_count;
  size_t comment_pos;

  comment_pos = 0;
  removed_count = 0;
  if (line_len == 0)
  {
    return 0;
  }

  if (*line == ';')
  {
    return line_len;
  }

  while (comment_pos < line_len)
  {
    comment_pos += strcspn(line + comment_pos, ";");
    if (comment_pos >= line_len)
    {
      break;
    }
    if (comment_pos != 0 && line[comment_pos - 1] == '\\')
    {
      strcpy(line + (comment_pos - 1), line + comment_pos);
      removed_count = removed_count + 1;
      line_len = line_len - 1;
    }
    else
    {
      line[comment_pos] = '\0';
      return (line_len - comment_pos) + removed_count;
    }
  }

  return removed_count;
}

// FUNCTION: SHANDALAR 0x004c83bb
int ReadIniTrimmedLine(FILE *file, char *line_out)
{
  int trimmed_len;
  int unused;
  size_t leading_trimmed_count;

  if (!file)
  {
    *line_out = '\0';
    return 0;
  }

  while (1)
  {
    if (fgets(line_out, 0x200, file) == (char *)0)
    {
      *line_out = '\0';
      return 0;
    }

    trimmed_len = (int)strlen(line_out);
    assert(trimmed_len < 0x1ff || line_out[trimmed_len - 1] == '\n',
           "D:\\Newmagic\\multiplayer\\sid\\iniFile.c", 0xc0,
           "Line too long in input file\n");
    while (trimmed_len > 0 &&
           (line_out[trimmed_len - 1] == '\n' ||
            line_out[trimmed_len - 1] == '\r'))
    {
      line_out[--trimmed_len] = '\0';
    }
    if (trimmed_len == 0)
    {
      continue;
    }

    leading_trimmed_count = strspn(line_out, " \t");
    trimmed_len -= leading_trimmed_count;
    trimmed_len -= (int)StripIniCommentSuffix(line_out + leading_trimmed_count, trimmed_len);

    if ((trimmed_len -= TrimIniTrailingWhitespace(line_out + leading_trimmed_count, trimmed_len)) == 0)
    {
      continue;
    }

    strcpy(line_out, line_out + leading_trimmed_count);
    return trimmed_len;
  }
}

// FUNCTION: SHANDALAR 0x004c7f20
int SeekIniLine(FILE *file, char *target_line)
{
  // GLOBAL: SHANDALAR 0x005b7758
  static char seek_line_005b7758[0x200];

  if (!file)
  {
    return 0;
  }

  fseek(file, 0, 0);

  while (ReadIniTrimmedLine(file, seek_line_005b7758))
  {
    if (_stricmp(seek_line_005b7758, target_line) == 0)
      return 1;
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004c7f85
int ReadIniSectionEntry(FILE *file, char *line_out)
{
  if (ReadIniTrimmedLine(file, line_out))
  {
    if (*line_out == '[')
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return 1;
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004c7fe3
char *BuildIniSectionHeader(char *section_name, char *unused)
{
  // GLOBAL: SHANDALAR 0x005b7958
  static char section_header_005b7958[0x100];
  void *idk = &unused;

  strcpy(section_header_005b7958, "[");
  strcat(section_header_005b7958, section_name);
  strcat(section_header_005b7958, "]");
  return section_header_005b7958;
}

// FUNCTION: SHANDALAR 0x004c8036
char *FindIniHeaderEntry(FILE *file, char *headers_section_name, char *entry_name)
{
  // GLOBAL: SHANDALAR 0x005b7a58
  static char entry_line_005b7a58[0x200];

  size_t entry_name_len;
  int found_headers_section;

  entry_name_len = strlen(entry_name);
  if (SeekIniLine(file, headers_section_name) == 0)
  {
    return (char *)0;
  }

  while (found_headers_section = ReadIniSectionEntry(file, entry_line_005b7a58))
  {
    if (_strnicmp(entry_line_005b7a58, entry_name, entry_name_len) == 0)
      break;
  }

  if (found_headers_section)
  {
    return entry_line_005b7a58;
  }
  else
  {
    return (char *)0;
  }
}

// FUNCTION: SHANDALAR 0x004c80d8
int *LoadIniEscapedStringTable(FILE *ini_file, char *section_name, char *scratch)
{
  int entry_count_remaining;
  int entry_count;
  int *entry_table;
  char *decoded_write_ptr;
  char decoded_line[0x200];
  char *entry_line;
  int *table_entry_ptr;

  entry_line = FindIniHeaderEntry(ini_file, "[headers]", section_name);
  if (entry_line == (char *)0)
  {
    return (int *)0;
  }

  entry_line = strchr(entry_line, ':');
  entry_count = atoi(entry_line + 1);
  if (SeekIniLine(ini_file, BuildIniSectionHeader(section_name, scratch)) == 0)
  {
    return (int *)0;
  }

  entry_table = (int *)malloc((size_t)entry_count << 2);
  table_entry_ptr = entry_table;
  memset(entry_table, 0, (size_t)entry_count << 2);
  strcpy(decoded_line, BuildIniSectionHeader(section_name, scratch));

  assert(SeekIniLine(ini_file, decoded_line), "D:\\Newmagic\\multiplayer\\sid\\iniFile.c", 0x7b,
         "Could not locate section header %s\n", decoded_line);

  while ((entry_count_remaining = entry_count--) != 0 && ReadIniSectionEntry(ini_file, decoded_line) != 0)
  {
    entry_line = decoded_line;
    decoded_write_ptr = entry_line;
    while (*entry_line != '\0')
    {
      if (*entry_line == '\\')
      {
        entry_line = entry_line + 1;
        switch (*entry_line)
        {
        case 'n':
          *decoded_write_ptr = '\n';
          decoded_write_ptr = decoded_write_ptr + 1;
          entry_line = entry_line + 1;
          break;
        case 'r':
          *decoded_write_ptr = '\r';
          decoded_write_ptr = decoded_write_ptr + 1;
          entry_line = entry_line + 1;
          break;
        case 'x':
          *decoded_write_ptr = (char)((IniHexDigitToInt((int)entry_line[1]) << 4) +
                                      IniHexDigitToInt((int)entry_line[2]));
          decoded_write_ptr = decoded_write_ptr + 1;
          entry_line = entry_line + 3;
          break;
        }
      }
      else
      {
        *decoded_write_ptr = *entry_line;
        entry_line = entry_line + 1;
        decoded_write_ptr = decoded_write_ptr + 1;
      }
    }

    *decoded_write_ptr = '\0';
    *table_entry_ptr = (int)malloc(strlen(decoded_line) + 2);
    strcpy((char *)*table_entry_ptr, decoded_line);
    table_entry_ptr = table_entry_ptr + 1;
  }

  return entry_table;
}
