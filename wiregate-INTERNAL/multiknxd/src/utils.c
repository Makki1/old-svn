/*
 * utils.c
 *
 * Copyright (C) 2012 - Michael Markstaller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * read in a textfile, return buffer
 * 
 */
char* file_read (const char* filename)
{
  FILE* fp;
  char* buffer;
  long  fsize;
  int fres;

  /* Open the file */
  fp = fopen(filename, "r");

  if (fp == NULL)
    {
      return NULL;
    }

  /* Get the size of the file */
  fseek(fp, 0, SEEK_END);
  fsize = ftell (fp) + 1;
  fseek(fp, 0, SEEK_SET);

  /* Allocate the buffer */
  buffer = calloc (fsize, sizeof (char));

  if (buffer == NULL)
    {
      fclose (fp);
      return NULL;
    }

  /* Read the file */
  fread(buffer, sizeof (char), fsize, fp);

  /* Close the file */
  fclose (fp);

  /* Return the buffer */
  return buffer;
}
