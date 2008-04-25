/* Conspire
 * Copyright (C) 2008 William Pitcock
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "signal_printer.h"
#include "text.h"
#include "modes.h"
#include "xchatc.h"

/* DCC */
#include "dcc.h"
#include "network.h"
#include "util.h"

/* actions */

void
signal_printer_action_public_highlight(gpointer *params)
{
	session *sess   = params[0];
	gchar *from     = params[1];
	gchar *text     = params[2];
	gchar *nickchar = params[3];
	
	EMIT_SIGNAL (XP_TE_HCHANACTION, sess, from, text, nickchar, NULL, 0);
}

/* Channels */

void
signal_printer_channel_created(gpointer *params)
{
	session *sess    = params[0];
	gchar *channel   = params[1];
	gchar *timestamp = params[2];

	EMIT_SIGNAL (XP_TE_CHANDATE, sess, channel, timestamp, NULL, NULL, 0);
}

void
signal_printer_channel_list_head(gpointer *params)
{
	server *serv = params[0];

	EMIT_SIGNAL (XP_TE_CHANLISTHEAD, serv->server_session, NULL, NULL, NULL, NULL, 0);
}

void
signal_printer_channel_modes(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_CHANMODES, sess, word[4], line[5], NULL, NULL, 0);
}

void
signal_printer_channel_join_error(gpointer *params)
{
	session *sess  = params[0];
	gchar *channel = params[1];
	gchar *error   = params[2];

	EMIT_SIGNAL (XP_TE_CHANNEL_JOIN_ERROR, sess, channel, error, NULL, NULL, 0);
}

void
signal_printer_channel_invited(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar *nick   = params[2];
	server *serv  = params[3];
			
	if (word[4][0] == ':')
		EMIT_SIGNAL (XP_TE_INVITED, sess, word[4] + 1, nick, serv->servername, NULL, 0);
	else
		EMIT_SIGNAL (XP_TE_INVITED, sess, word[4], nick, serv->servername, NULL, 0);
}

/* DCC */

void
signal_printer_dcc_abort(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;

	switch (dcc->type) {
		case TYPE_CHATSEND:
		case TYPE_CHATRECV:
			EMIT_SIGNAL (XP_TE_DCCCHATABORT, serv->front_session, dcc->nick, NULL, NULL, NULL, 0);
			break;
		case TYPE_SEND:
			EMIT_SIGNAL (XP_TE_DCCSENDABORT, serv->front_session, dcc->nick, NULL, NULL, NULL, 0);
			break;
		case TYPE_RECV:
			EMIT_SIGNAL (XP_TE_DCCRECVABORT, serv->front_session, dcc->nick, NULL, NULL, NULL, 0);
			break;
	}
}

void
signal_printer_dcc_chat_duplicate(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	EMIT_SIGNAL (XP_TE_DCCCHATREOFFER, sess, nick, NULL, NULL, NULL, 0);
}

void
signal_printer_dcc_chat_failed(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *portbuf = params[1];
	gchar *error = params[2];

	EMIT_SIGNAL (XP_TE_DCCCHATF, serv->front_session, dcc->nick, net_ip(dcc->addr), portbuf, error, 0);
	dcc_close(dcc, STAT_FAILED, FALSE);
}

void
signal_printer_dcc_chat_offer(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	EMIT_SIGNAL (XP_TE_DCCCHATOFFERING, sess, nick, NULL, NULL, NULL, 0);
}

void
signal_printer_dcc_chat_request(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	EMIT_SIGNAL (XP_TE_DCCCHATOFFER, sess->server->front_session, nick, NULL, NULL, NULL, 0);
}

void
signal_printer_dcc_connected(gpointer *params)
{
	struct DCC *dcc  = params[0];
	server *serv = dcc->serv;
	gchar  *host = params[1];

	switch (dcc->type) {
		case TYPE_SEND:
			EMIT_SIGNAL (XP_TE_DCCCONSEND, serv->front_session, dcc->nick, host, dcc->file, NULL, 0);
			break;
		case TYPE_RECV:
			EMIT_SIGNAL (XP_TE_DCCCONRECV, serv->front_session, dcc->nick, host, dcc->file, NULL, 0);
			break;
		case TYPE_CHATRECV:
			EMIT_SIGNAL (XP_TE_DCCCONCHAT, serv->front_session, dcc->nick, host, NULL, NULL, 0);
			break;
	}
}

void
signal_printer_dcc_failed(gpointer *params)
{
	struct DCC *dcc = params[0];
	gchar *error = params[1];
	server *serv = dcc->serv;
	gchar *type = g_strdup(dcctypes[dcc->type]);

	EMIT_SIGNAL (XP_TE_DCCCONFAIL, serv->front_session, type, dcc->nick, error, NULL, 0);
	g_free(type);
}

void
signal_printer_dcc_file_complete(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *buf;

	dcc_close (dcc, STAT_DONE, FALSE);
	dcc_calc_average_cps (dcc);	/* this must be done _after_ dcc_close, or dcc_remove_from_sum will see the wrong value in dcc->cps */
	buf = g_strdup_printf("%d", dcc->cps);

	EMIT_SIGNAL (XP_TE_DCCRECVCOMP, serv->front_session, dcc->file, dcc->destfile, dcc->nick, buf, 0);
	g_free(buf);
}

void
signal_printer_dcc_file_error(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *error = params[1];

	EMIT_SIGNAL (XP_TE_DCCFILEERR, serv->front_session, dcc->destfile, error, NULL, NULL, 0);
	dcc_close (dcc, STAT_FAILED, FALSE);
}

void
signal_printer_dcc_file_request(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	gchar *file = params[2];
	gchar *tbuf = params[3];
	EMIT_SIGNAL (XP_TE_DCCSENDOFFER, sess->server->front_session, nick, file, tbuf, tbuf + 24, 0);
}

void
signal_printer_dcc_file_renamed(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *old = params[1];

	EMIT_SIGNAL (XP_TE_DCCRENAME, serv->front_session, old, dcc->destfile, NULL, NULL, 0);
}

void
signal_printer_dcc_file_resume(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	struct DCC *dcc = params[2];
	gchar *tbuf = params[3];

	EMIT_SIGNAL (XP_TE_DCCRESUMEREQUEST, sess, nick, file_part (dcc->file), tbuf, NULL, 0);
}

void
signal_printer_dcc_generic_offer(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	gchar *data = params[2];

	EMIT_SIGNAL (XP_TE_DCCGENERICOFFER, sess->server->front_session, data, nick, NULL, NULL, 0);
}

void
signal_printer_dcc_invalid(gpointer *params)
{
	struct session *sess = params[0];
	EMIT_SIGNAL (XP_TE_DCCIVAL, sess, NULL, NULL, NULL, NULL, 0);
}

void
signal_printer_dcc_list_start(gpointer *params)
{
	struct session *sess = params[0];

	EMIT_SIGNAL (XP_TE_DCCHEAD, sess, NULL, NULL, NULL, NULL, 0);
}

void
signal_printer_dcc_malformed(gpointer *params)
{
	struct session *sess = params[0];
	gchar *nick = params[1];
	gchar *data = params[2];
	EMIT_SIGNAL (XP_TE_MALFORMED, sess, nick, data, NULL, NULL, 0);
}

void
signal_printer_dcc_recv_error(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *error = params[1];

	EMIT_SIGNAL (XP_TE_DCCRECVERR, serv->front_session, dcc->file, dcc->destfile, dcc->nick, error, 0);
	dcc_close (dcc, STAT_FAILED, FALSE);
}

void
signal_printer_dcc_send_complete(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *buf;

	/* force 100% ack for >4 GB */
	dcc->ack = dcc->size;
	dcc_close (dcc, STAT_DONE, FALSE);
	dcc_calc_average_cps (dcc);

	buf = g_strdup_printf("%d", dcc->cps);
	EMIT_SIGNAL (XP_TE_DCCSENDCOMP, serv->front_session, file_part(dcc->file), dcc->nick, buf, NULL, 0);
	g_free(buf);
}

void
signal_printer_dcc_send_failed(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *error = params[1];

	EMIT_SIGNAL (XP_TE_DCCSENDFAIL, serv->front_session, file_part(dcc->file), dcc->nick, error, NULL, 0);
	dcc_close (dcc, STAT_FAILED, FALSE);
}

void
signal_printer_dcc_send_request(gpointer *params)
{
	struct session *sess = params[0];
	struct DCC *dcc = params[1];
	gchar *to = params[2];
	
	EMIT_SIGNAL (XP_TE_DCCOFFER, sess, file_part(dcc->file), to, dcc->file, NULL, 0);
}

void
signal_printer_dcc_stoned(gpointer *params)
{
	struct DCC *dcc = params[0];
	server *serv = dcc->serv;
	gchar *type = g_strdup(dcctypes[dcc->type]);

	EMIT_SIGNAL (XP_TE_DCCTOUT, serv->front_session, type, file_part(dcc->file), dcc->nick, NULL, 0);
	dcc_close(dcc, STAT_ABORTED, FALSE); 
}

/* non-query private messages */

void
signal_printer_message_private(gpointer *params)
{
	session *sess  = params[0];
	gchar *nick    = params[1];
	gchar *message = params[2];
	gchar *idtext  = params[3];
	
	if (sess->type == SESS_DIALOG) {
		EMIT_SIGNAL (XP_TE_DPRIVMSG, sess, nick, message, idtext, NULL, 0);
	} else {
		EMIT_SIGNAL (XP_TE_PRIVMSG, sess, nick, message, idtext, NULL, 0);
	}
}

/* queries */

void
signal_printer_query_open(gpointer *params)
{
	session *sess = params[0];

	EMIT_SIGNAL (XP_TE_OPENDIALOG, sess, NULL, NULL, NULL, NULL, 0);
}

/* server */

void
signal_printer_server_connected(gpointer *params)
{
	server *serv = params[0];

	EMIT_SIGNAL (XP_TE_CONNECTED, serv->server_session, NULL, NULL, NULL, NULL, 0);
}

void
signal_printer_server_stoned(gpointer *params)
{
	server *serv = params[0];
	gchar *tbuf;

	tbuf = g_strdup_printf("%d", GPOINTER_TO_INT(params[1]));
	EMIT_SIGNAL(XP_TE_PINGTIMEOUT, serv->server_session, tbuf, NULL, NULL, NULL, 0);
	serv->auto_reconnect(serv, FALSE, -1);
	g_free(tbuf);
}

void
signal_printer_server_dns_lookup(gpointer *params)
{
	session *sess = params[0];
	gchar *hostname = params[1];

	EMIT_SIGNAL (XP_TE_SERVERLOOKUP, sess, hostname, NULL, NULL, NULL, 0);
}

void
signal_printer_server_text(gpointer *params)
{
	session *sess  = params[0];
	gchar *text    = params[1];
	gchar *context = params[2];

	EMIT_SIGNAL (XP_TE_SERVTEXT, sess, text, context, NULL, NULL, 0);
}

void
signal_printer_server_motd(gpointer *params)
{
	session *sess = params[0];
	gchar *text   = params[1];

	EMIT_SIGNAL (XP_TE_MOTD, sess, text, NULL, NULL, NULL, 0);
}

void
signal_printer_server_wallops(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];
	gchar *text   = params[2];

	EMIT_SIGNAL (XP_TE_WALLOPS, sess, nick, text, NULL, NULL, 0);
}

void
signal_printer_server_error(gpointer *params)
{
	session *sess = params[0];
	gchar *error  = params[1];
	
	EMIT_SIGNAL (XP_TE_SERVERERROR, sess, error, NULL, NULL, NULL, 0);
}

void
signal_printer_server_notice(gpointer *params)
{
	session *sess = params[0];
	gchar *text   = params[1];

	EMIT_SIGNAL (XP_TE_SERVNOTICE, sess, text, sess->server->servername, NULL, NULL, 0);
}

void
signal_printer_server_kill(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_KILL, sess, nick, line[5], NULL, NULL, 0);
}

/* whois */

void
signal_printer_whois_server(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];
	gchar *server = params[2];
	
	EMIT_SIGNAL (XP_TE_WHOIS_SERVER, sess, nick, server, NULL, NULL, 0);
}

void
signal_printer_whois_name(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_NAME, sess, word[4], word[5], word[6], line[8] + 1, 0);
}

void
signal_printer_whois_idle(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];
	gchar *idle   = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_IDLE, sess, nick, idle, NULL, NULL, 0);
}

void
signal_printer_whois_idle_signon(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];
	gchar *idle   = params[2];
	gchar *signon = params[3];

	EMIT_SIGNAL (XP_TE_WHOIS_IDLE_SIGNON, sess, nick, idle, signon, NULL, 0);
}

void
signal_printer_whois_end(gpointer *params)
{
	session *sess = params[0];
	gchar *nick   = params[1];

	EMIT_SIGNAL (XP_TE_WHOIS_END, sess, nick, NULL, NULL, NULL, 0);
}

void
signal_printer_whois_oper(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_OPER, sess, word[4], line[5] + 1, NULL, NULL, 0);
}

void
signal_printer_whois_channels(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_CHANNELS, sess, word[4], line[5] + 1, NULL, NULL, 0);
}

void
signal_printer_whois_identified(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_ID, sess, word[4], line[5] + 1, NULL, NULL, 0);
}

void
signal_printer_whois_authenticated(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_AUTH, sess, word[4], line[6] + 1, word[5], NULL, 0);
}

void
signal_printer_whois_generic(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	gchar **line  = params[2];

	EMIT_SIGNAL (XP_TE_WHOIS_SPECIAL, sess, word[4], (line[5][0] == ':') ? line[5] + 1 : line[5], word[2], NULL, 0);
}

/* sasl -- temporary */
void
signal_printer_sasl_complete(gpointer *params)
{
	session *sess  = params[0];
	gchar *account = params[1];

	EMIT_SIGNAL (XP_TE_SASL_AUTH, sess, account, NULL, NULL, NULL, 0);
}

/* ctcp */
void
signal_printer_ctcp_inbound(gpointer *params)
{
	session *sess = params[0];
	gchar *msg    = params[1];
	gchar *nick   = params[2];
	gchar *to     = params[3];

	if(!is_channel(sess->server, to))
	{
		EMIT_SIGNAL(XP_TE_CTCPGEN, sess->server->front_session, msg, nick, NULL, NULL, 0);
	}
	else
	{
		session *chansess = find_channel(sess->server, to);
		if (!chansess)
			chansess = sess;

		EMIT_SIGNAL(XP_TE_CTCPGENC, chansess, msg, nick, to, NULL, 0);
	}
}

/* user-sent signals */

void
signal_printer_user_invite(gpointer *params)
{
	session *sess = params[0];
	gchar **word  = params[1];
	server *serv  = params[2];

	EMIT_SIGNAL (XP_TE_UINVITE, sess, word[4], word[5], serv->servername, NULL, 0);
}

void
signal_printer_init(void)
{
	/* actions */
	signal_attach("action public highlight", signal_printer_action_public_highlight);

	/* Channels */
	signal_attach("channel created",    signal_printer_channel_created);
	signal_attach("channel list head",  signal_printer_channel_list_head);
	signal_attach("channel modes",      signal_printer_channel_modes);
	signal_attach("channel join error", signal_printer_channel_join_error);
	signal_attach("channel invited",    signal_printer_channel_invited);

	/* DCC */
	signal_attach("dcc abort",          signal_printer_dcc_abort);
	signal_attach("dcc chat duplicate", signal_printer_dcc_chat_duplicate);
	signal_attach("dcc chat failed",    signal_printer_dcc_chat_failed);
	signal_attach("dcc chat offer",     signal_printer_dcc_chat_offer);
	signal_attach("dcc chat request",   signal_printer_dcc_chat_request);
	signal_attach("dcc connected",      signal_printer_dcc_connected);
	signal_attach("dcc failed",         signal_printer_dcc_failed);
	signal_attach("dcc file complete",  signal_printer_dcc_file_complete);
	signal_attach("dcc file error",     signal_printer_dcc_file_error);
	signal_attach("dcc file request",   signal_printer_dcc_file_request);
	signal_attach("dcc file renamed",   signal_printer_dcc_file_renamed);
	signal_attach("dcc file resume",    signal_printer_dcc_file_resume);
	/* if anyone can figure out why this would be necessary, I'd love to know. */
	signal_attach("dcc generic offer",  signal_printer_dcc_generic_offer);
	signal_attach("dcc invalid",        signal_printer_dcc_invalid);
	signal_attach("dcc list start",     signal_printer_dcc_list_start);
	signal_attach("dcc malformed",      signal_printer_dcc_malformed);
	signal_attach("dcc recv error",     signal_printer_dcc_recv_error);
	signal_attach("dcc send complete",  signal_printer_dcc_send_complete);
	signal_attach("dcc send failed",    signal_printer_dcc_send_failed);
	signal_attach("dcc send request",   signal_printer_dcc_send_request);
	signal_attach("dcc stoned",         signal_printer_dcc_stoned);

	/* non-query messages */
	signal_attach("message private",    signal_printer_message_private);

	/* queries */
	signal_attach("query open",         signal_printer_query_open);

	/* server */
	signal_attach("server connected",   signal_printer_server_connected);
	signal_attach("server dns lookup",  signal_printer_server_dns_lookup);
	signal_attach("server stoned",      signal_printer_server_stoned);
	signal_attach("server text",        signal_printer_server_text);
	signal_attach("server motd",        signal_printer_server_motd);
	signal_attach("server wallops",     signal_printer_server_wallops);
	signal_attach("server error",       signal_printer_server_error);
	signal_attach("server notice",      signal_printer_server_notice);

	/* whois */
	signal_attach("whois server",        signal_printer_whois_server);
	signal_attach("whois name",          signal_printer_whois_name);
	signal_attach("whois idle",          signal_printer_whois_idle);
	signal_attach("whois idle signon",   signal_printer_whois_idle_signon);
	signal_attach("whois end",           signal_printer_whois_end);
	signal_attach("whois oper",          signal_printer_whois_oper);
	signal_attach("whois channels",      signal_printer_whois_channels);
	signal_attach("whois authenticated", signal_printer_whois_authenticated);
	signal_attach("whois identified",    signal_printer_whois_identified);
	signal_attach("whois generic",       signal_printer_whois_generic);

	/* sasl */
	signal_attach("sasl complete",      signal_printer_sasl_complete);

	/* ctcp */
	signal_attach("ctcp inbound",       signal_printer_ctcp_inbound);

	/* user-sent signals */
	signal_attach("user invite",        signal_printer_user_invite);
}
