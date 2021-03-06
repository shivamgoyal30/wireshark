/* tap_camelcounter.c
 * camel message counter for tshark
 * Copyright 2006 Florent DROUIN
 *
 * This part of code is extracted from tap-h225counter.c from Lars Roland
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epan/packet.h"
#include "epan/tap.h"
#include "epan/value_string.h"
#include "epan/stat_tap_ui.h"
#include "epan/dissectors/packet-camel.h"

void register_tap_listener_camelcounter(void);

/* used to keep track of the statistics for an entire program interface */
struct camelcounter_t {
  char *filter;
  guint32 camel_msg[camel_MAX_NUM_OPR_CODES];
};


static void camelcounter_reset(void *phs)
{
  struct camelcounter_t * p_counter= ( struct camelcounter_t *) phs;
  memset(p_counter, 0, sizeof(struct camelcounter_t));
}

static int camelcounter_packet(void *phs,
                               packet_info *pinfo _U_,
                               epan_dissect_t *edt _U_,
                               const void *phi)
{
  struct camelcounter_t * p_counter =(struct camelcounter_t *)phs;
  const struct camelsrt_info_t * pi = (const struct camelsrt_info_t *)phi;
  if (pi->opcode != 255)
    p_counter->camel_msg[pi->opcode]++;

  return 1;
}


static void camelcounter_draw(void *phs)
{
  struct camelcounter_t * p_counter= (struct camelcounter_t *)phs;
  int i;
  gchar *tmp_str;
  printf("\n");
  printf("CAMEL Message and Response Status Counter:\n");
  printf("------------------------------------------\n");

  for (i=0; i<camel_MAX_NUM_OPR_CODES; i++) {
    /* Message counter */
    if (p_counter->camel_msg[i] != 0) {
      tmp_str = val_to_str_wmem(NULL, i, camel_opr_code_strings, "Unknown message (%d)");
      printf("%30s ", tmp_str);
      wmem_free(NULL, tmp_str);
      printf("%6d\n", p_counter->camel_msg[i]);
    }
  } /* Message Type */
  printf("------------------------------------------\n");
}

static void camelcounter_init(const char *opt_arg, void *userdata _U_)
{
  struct camelcounter_t *p_camelcounter;
  GString *error_string;

  p_camelcounter = g_new(struct camelcounter_t, 1);
  if (!strncmp(opt_arg, "camel,counter,", 13)) {
    p_camelcounter->filter = g_strdup(opt_arg+13);
  } else {
    p_camelcounter->filter = NULL;
  }

  camelcounter_reset(p_camelcounter);

  error_string = register_tap_listener("CAMEL",
                                     p_camelcounter,
                                     p_camelcounter->filter,
                                     0,
                                     NULL,
                                     camelcounter_packet,
                                     camelcounter_draw);

  if (error_string) {
    /* error, we failed to attach to the tap. clean up */
    g_free(p_camelcounter->filter);
    g_free(p_camelcounter);

    fprintf(stderr, "tshark: Couldn't register camel,counter tap: %s\n",
            error_string->str);
    g_string_free(error_string, TRUE);
    exit(1);
  }
}

static stat_tap_ui camelcounter_ui = {
  REGISTER_STAT_GROUP_GENERIC,
  NULL,
  "camel,counter",
  camelcounter_init,
  -1,
  0,
  NULL
};

void  /* Next line mandatory */
register_tap_listener_camelcounter(void)
{
  register_stat_tap_ui(&camelcounter_ui, NULL);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
