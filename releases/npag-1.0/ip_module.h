/*
 * This file is part of
 * npag - Network Packet Generator
 * Copyright (C) 2005 Christian Bannes, University of T�bingen,
 * Germany
 * 
 * npag is free software; you can redistribute it and/or
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA  02110-1301, USA.
 */

void init_rawsocket		__P((sock_descriptor_t *fd, config_t*));
void init_rawsocket6	__P((sock_descriptor_t *fd, config_t*));

void fill_ip4hdr		__P((packet_buffer_t*, config_t*));
void fill_ip6hdr		__P((packet_buffer_t*, config_t*));

void set_ipsockopts		__P((packet_buffer_t*, config_t*));

