////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Text User Interface (TUI)
//
//  author(s):
//  ENDESGA - https://x.com/ENDESGA | https://bsky.app/profile/endesga.bsky.social
//
//  https://github.com/H-language/TUI
//  2026 - CC0 - FOSS forever
//

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - DEPENDENCIES
//

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#define PEP_IMPLEMENTATION
#include <pep.h>
#include <C7H16.h>
#include "assets/font_pep.h"

#pragma endregion dependencies

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - CONSTANTS
//

#define TUI_NAME "TUI"

////////////////////////////////////////////////////////////////
#pragma region - version

#define TUI_VERSION_MAJOR 0
#define TUI_VERSION_MINOR 1
#define TUI_VERSION_PATCH 0
#define TUI_VERSION_COMMIT 0
#define TUI_VERSION AS_BYTES( TUI_VERSION_MAJOR ) "." AS_BYTES( TUI_VERSION_MINOR ) "." AS_BYTES( TUI_VERSION_PATCH ) "-" AS_BYTES( TUI_VERSION_COMMIT )

#pragma endregion version

////////////////////////////////////////////////////////////////
#pragma region - limits

#define TUI_max_line_size 128
#define TUI_max_lines 64
#define TUI_max_bytes 1024
#define TUI_max_buttons 256
#define TUI_max_input_bytes 128

#pragma endregion limits

#pragma endregion constants

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - DECLARATIONS
//

////////////////////////////////////////////////////////////////
#pragma region - color

group( TUI_color )
{
	TUI_color_none,
	TUI_color_red,
	TUI_color_yellow,
	TUI_color_green,
	TUI_color_cyan,
	TUI_color_blue,
	TUI_color_magenta,
	TUI_color_white,

	TUI_colors_count
};

perm pixel const _TUI_colors[ TUI_colors_count ] =
	{
		pixel_invalid,
		pixel_red,
		pixel_yellow,
		pixel_green,
		pixel_cyan,
		pixel_blue,
		pixel_magenta,
		pixel_white,
	};

perm n1 const _TUI_lut[] =
	{
		[ 'r' - '0' ] = TUI_color_red,
		[ 'y' - '0' ] = TUI_color_yellow,
		[ 'g' - '0' ] = TUI_color_green,
		[ 'c' - '0' ] = TUI_color_cyan,
		[ 'b' - '0' ] = TUI_color_blue,
		[ 'm' - '0' ] = TUI_color_magenta,
		[ 'w' - '0' ] = TUI_color_white,
	};

#pragma endregion color

////////////////////////////////////////////////////////////////
#pragma region - byte

type( TUI_byte )
{
	byte byte;
	TUI_color fore bits( TUI_colors_count );
	TUI_color back bits( TUI_colors_count );
	n1 x bits( TUI_max_line_size );
	n1 y bits( TUI_max_lines );
};

#pragma endregion byte

////////////////////////////////////////////////////////////////
#pragma region - button

type( TUI_button )
{
	fn_ref( anon, callback );
	anon ref data;
	n1 x bits( TUI_max_line_size );
	n1 y bits( TUI_max_lines );
	n1 size bits( TUI_max_line_size );
};

#pragma endregion button

////////////////////////////////////////////////////////////////
#pragma region - TUI

global
{
	window ref window_ref;
	view ref view_ref;
	fn_ref( anon, fn_print );
	font font_main;
	TUI_byte bytes[ TUI_max_bytes ];
	TUI_button buttons[ TUI_max_buttons ];
	n2 row_start[ TUI_max_lines + 1 ];
	byte input_bytes[ TUI_max_input_bytes ];

	n2 bytes_count bits( TUI_max_bytes );
	n2 update_start bits( TUI_max_bytes );
	n1 button_count bits( TUI_max_buttons );
	n1 hover_target bits( TUI_max_buttons );
	n1 current_hover bits( TUI_max_buttons );
	n1 input_bytes_count bits( TUI_max_input_bytes );
	TUI_color fore bits( TUI_colors_count );
	TUI_color back bits( TUI_colors_count );
	n1 x bits( TUI_max_line_size );
	n1 y bits( TUI_max_lines );
	flag redraw bits_flag;
	flag reprint bits_flag;
}
TUI;

#pragma endregion TUI

#pragma endregion declarations

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - DEFINITIONS
//

////////////////////////////////////////////////////////////////
#pragma region - TUI

////////////////////////////////
#pragma region | TUI / hidden

////////////////
#pragma region | - draw

fn _TUI_set_byte( TUI_byte const tui_byte, flag const is_hovered )
{
	out_if( tui_byte.byte is ' ' and tui_byte.back is TUI_color_none and is_hovered is no );

	i2x2 const font_size = to_i2x2( TUI.font_main.letter_size );
	i2x2 const pos = i2x2( tui_byte.x * font_size.w, tui_byte.y * font_size.h );

	TUI_color const fore = pick( is_hovered, tui_byte.back, tui_byte.fore );
	TUI_color const back = pick( is_hovered, pick( tui_byte.fore is TUI_color_none, TUI_color_white, tui_byte.fore ), tui_byte.back );

	if( back isnt TUI_color_none )
	{
		fill_area( pos, i2x2_add( pos, i2x2_sub_i2( font_size, 1 ) ), _TUI_colors[ back ] );
	}

	out_if( tui_byte.byte is ' ' );

	set_byte( TUI.font_main, tui_byte.byte, pos, anchor_left_top, _TUI_colors[ fore ] );
}

fn _TUI_view_draw( view ref const view_ref )
{
	program.current_canvas_ref = ref_of( view_ref->canvas );

	n2x2 const font_size = TUI.font_main.letter_size;
	flag const has_hover = TUI.hover_target < TUI.button_count;
	TUI_button const hover_button = TUI.buttons[ TUI.hover_target ];
	n1 const hover_x_end = hover_button.x + hover_button.size;

	if( TUI.redraw )
	{
		clear();
		TUI.update_start = 0;
		TUI.current_hover = TUI.hover_target;
		TUI.redraw = no;

		if( has_hover )
		{
			fill_area( i2x2( hover_button.x * font_size.w, hover_button.y * font_size.h ), i2x2( ( hover_x_end * font_size.w ) - 1, ( ( hover_button.y + 1 ) * font_size.h ) - 1 ), pixel_white );
		}
	}
	else
	{
		if( TUI.current_hover isnt TUI.hover_target )
		{

			flag const has_old = TUI.current_hover < TUI.button_count;
			TUI_button const ref const old_button_ref = ref_of( TUI.buttons[ TUI.current_hover ] );
			n1 const old_x_end = old_button_ref->x + old_button_ref->size;

			if( has_old )
			{
				fill_area( i2x2( old_button_ref->x * font_size.w, old_button_ref->y * font_size.h ), i2x2( ( old_x_end * font_size.w ) - 1, ( ( old_button_ref->y + 1 ) * font_size.h ) -1 ), pixel_black );

				n2 const button_start = TUI.row_start[ old_button_ref->y ];
				n2 const button_end = TUI.row_start[ old_button_ref->y + 1 ];
				range( byte_id, button_start, button_end - 1 )
				{
					TUI_byte const this_byte = TUI.bytes[ byte_id ];
					if( this_byte.x >= old_button_ref->x and this_byte.x < old_x_end )
					{
						_TUI_set_byte( this_byte, no );
					}
				}
			}

			if( has_hover )
			{
				fill_area( i2x2( hover_button.x * font_size.w, hover_button.y * font_size.h ), i2x2( ( hover_x_end * font_size.w ) -1, ( ( hover_button.y + 1 ) * font_size.h ) -1 ), pixel_white );

				n2 const s = TUI.row_start[ hover_button.y ];
				n2 const e = TUI.row_start[ hover_button.y + 1 ];
				range( byte_id, s, e - 1 )
				{
					TUI_byte const this_byte = TUI.bytes[ byte_id ];
					if( this_byte.x >= hover_button.x and this_byte.x < hover_x_end )
					{
						_TUI_set_byte( this_byte, yes );
					}
				}
			}

			TUI.current_hover = TUI.hover_target;
		}
		else if( TUI.update_start > TUI.bytes_count - 1 )
		{
			fill_area( i2x2( TUI.x * font_size.w, TUI.y * font_size.h ), i2x2( ( ( TUI.x + ( TUI.update_start - TUI.bytes_count ) ) * font_size.w ) -1, ( ( TUI.y + 1 ) * font_size.h ) -1 ), pixel_black );
		}
	}

	range( byte_id, TUI.update_start, TUI.bytes_count - 1 )
	{
		TUI_byte const this_byte = TUI.bytes[ byte_id ];
		flag const h = has_hover and this_byte.y is hover_button.y and this_byte.x >= hover_button.x and this_byte.x < hover_x_end;
		_TUI_set_byte( this_byte, h );
	}

	TUI.update_start = TUI.bytes_count;
}

fn _TUI_set_scale( r4 const scale )
{
	TUI.view_ref->scale = r4x2( scale, scale );
	r4 const pos = AVG( r4( TUI.font_main.letter_size.w ), r4( TUI.font_main.letter_size.h ) ) * 0.5 * scale;
	TUI.view_ref->pos = r4x2( pos, pos );
}

#pragma endregion

////////////////
#pragma region | - print

fn _TUI_print_byte( byte const byte )
{
	TUI.bytes[ TUI.bytes_count ].byte = byte;
	TUI.bytes[ TUI.bytes_count ].fore = TUI.fore;
	TUI.bytes[ TUI.bytes_count ].back = TUI.back;
	TUI.bytes[ TUI.bytes_count ].x = TUI.x;
	TUI.bytes[ TUI.bytes_count ].y = TUI.y;
	TUI.bytes_count += 1;
	TUI.row_start[ TUI.y + 1 ] = TUI.bytes_count;
	TUI.x += 1;
}

fn _TUI_print( byte const ref const bytes )
{
	out_if_nothing( bytes );

	n4 byte_id = 0;
	loop
	{
		byte const this_byte = bytes[ byte_id ];
		out_if( this_byte is eof_byte );

		if( this_byte is newline_byte )
		{
			TUI.x = 0;
			TUI.y += 1;
			TUI.row_start[ TUI.y ] = TUI.bytes_count;
			byte_id += 1;
			next;
		}

		if( this_byte is '<' and bytes[ byte_id + 1 ] and bytes[ byte_id + 2 ] is '>' )
		{
			byte const tag_byte = bytes[ byte_id + 1 ];

			if( tag_byte is '0' )
			{
				TUI.fore = TUI_color_none;
				TUI.back = TUI_color_none;
				byte_id += 3;
				next;
			}

			if( tag_byte >= 'a' and tag_byte <= 'z' )
			{
				TUI.fore = _TUI_lut[ tag_byte - '0' ];
				byte_id += 3;
				next;
			}

			if( tag_byte >= 'A' and tag_byte <= 'Z' )
			{
				TUI.back = _TUI_lut[ tag_byte + 32 - '0' ];
				byte_id += 3;
				next;
			}
		}

		if( TUI.bytes_count < TUI_max_bytes and TUI.x < TUI_max_line_size and TUI.y < TUI_max_lines )
		{
			_TUI_print_byte( this_byte );
		}

		byte_id += 1;
	}
}

fn _TUI_print_button( byte const ref const bytes, fn_ref( anon, callback ), anon ref data )
{
	out_if( TUI.button_count >= TUI_max_buttons - 1 );

	n1 const x_prev = TUI.x;

	_TUI_print( bytes );

	TUI.buttons[ TUI.button_count ].x = x_prev;
	TUI.buttons[ TUI.button_count ].y = TUI.y;
	TUI.buttons[ TUI.button_count ].size = TUI.x - x_prev;
	TUI.buttons[ TUI.button_count ].callback = callback;
	TUI.buttons[ TUI.button_count ].data = data;
	TUI.button_count += 1;
}

#pragma endregion

////////////////
#pragma region | - command

embed flag _TUI_button_match( n1 const button_id, byte const ref const bytes_match, n1 const bytes_match_count )
{
	TUI_button const ref const button_ref = ref_of( TUI.buttons[ button_id ] );
	out_if( button_ref->size < bytes_match_count ) no;

	n2 const row_start = TUI.row_start[ button_ref->y ];
	n2 const row_end = TUI.row_start[ button_ref->y + 1 ];
	n1 const x_end = button_ref->x + button_ref->size;
	n2 match_id = 0;
	range( byte_id, row_start, row_end - 1 )
	{
		TUI_byte const this_byte = TUI.bytes[ byte_id ];
		if( this_byte.x >= button_ref->x and this_byte.x < x_end )
		{
			if( this_byte.byte is bytes_match[ match_id ] )
			{
				match_id += 1;
				skip_if( match_id >= bytes_match_count );
			}
			else
			{
				out no;
			}
		}
	}
	out yes;
}

embed flag _TUI_execute_word( byte const ref const word, n1 const word_len )
{
	iter_inv( button_id, TUI.button_count )
	{
		TUI_button const ref const button_ref = ref_of( TUI.buttons[ button_id ] );
		if( button_ref->data is TUI.input_bytes ) next;

		if( button_ref->size is word_len and _TUI_button_match( button_id, word, word_len ) )
		{
			TUI.hover_target = button_id;
			call( TUI.buttons[ TUI.hover_target ].callback );
			out yes;
		}
	}

	iter_inv( button_id, TUI.button_count )
	{
		TUI_button const ref const button_ref = ref_of( TUI.buttons[ button_id ] );
		if( button_ref->data is TUI.input_bytes )
		{
			TUI.hover_target = button_id;
			call( TUI.buttons[ TUI.hover_target ].callback );
			out yes;
		}
	}
	out no;
}

#pragma endregion

////////////////
#pragma region | - state

fn _TUI_clear()
{
	TUI.x = 0;
	TUI.y = 0;
	TUI.bytes_count = 0;
	TUI.button_count = 0;
	TUI.redraw = yes;
	TUI.input_bytes[ 0 ] = eof_byte;
	TUI.input_bytes_count = 0;

	iter( i, TUI_max_lines + 1 )
	{
		TUI.row_start[ i ] = 0;
	}

	TUI.hover_target = TUI_max_buttons - 1;
	if_something( TUI.window_ref )
	{
		window_ref_set_cursor( TUI.window_ref, cursor_arrow );
	}
}

fn _TUI_call_reprint()
{
	_TUI_clear();
	call( TUI.fn_print );
	TUI.reprint = no;
}

#pragma endregion

////////////////
#pragma region | - tick

fn _TUI_window_tick( window ref const window_ref )
{
	if( program.fps > 0 )
	{
		TUI.reprint = yes;
	}

	n2x2 const mouse_cell = to_n2x2( r4x2_div( TUI.view_ref->mouse, to_r4x2( TUI.font_main.letter_size ) ) );
	n1 new_hover = TUI.button_count;

	flag update = no;

	iter( button_id, TUI.button_count )
	{
		TUI_button const ref const b = ref_of( TUI.buttons[ button_id ] );
		if( mouse_cell.y is b->y and mouse_cell.x >= b->x and mouse_cell.x < b->x + b->size )
		{
			new_hover = button_id;
			skip;
		}
	}

	if( new_hover isnt TUI.hover_target )
	{
		TUI.hover_target = new_hover;
		update = yes;
	}

	if( TUI.hover_target < TUI.button_count )
	{
		window_ref_set_cursor( window_ref, cursor_hand );
	}
	else if( update is yes )
	{
		window_ref_set_cursor( window_ref, cursor_arrow );
	}

	// input

	if( input_pressed( keyboard_f11 ) )
	{
		window_ref_toggle_fullscreen( window_ref );
	}

	if( input_held( keyboard_ctrl ) )
	{
		i1 const delta = pick( input_scrolled_up, 1, pick( input_scrolled_down, -1, 0 ) );
		if( delta isnt 0 )
		{
			r4 const scale = r4( i1_min( i1_max( i1( TUI.view_ref->scale.w ) + delta, 1 ), 10 ) );
			_TUI_set_scale( scale );
			TUI.reprint = yes;
		}
	}

	if( input_pressed( mouse_left ) and TUI.hover_target < TUI.button_count )
	{
		call( TUI.buttons[ TUI.hover_target ].callback );
		update = yes;
	}

	iter( input_index, window_ref->input_bytes_count )
	{
		byte const input_byte = window_ref->input_bytes[ input_index ];

		skip_if( input_byte is ' ' and TUI.input_bytes_count <= 0 );
		skip_if( TUI.input_bytes_count >= TUI_max_input_bytes - 1 );

		if( input_byte < ' ' ) next;

		_TUI_print_byte( input_byte );

		TUI.input_bytes[ TUI.input_bytes_count++ ] = input_byte;
		TUI.input_bytes[ TUI.input_bytes_count ] = eof_byte;
		update = yes;
	}

	if( window_ref->input_bytes_count > 0 and window_ref->input_bytes[ 0 ] is '\b' and TUI.input_bytes_count > 0 )
	{
		TUI.input_bytes_count -= 1;
		TUI.input_bytes[ TUI.input_bytes_count ] = eof_byte;
		TUI.bytes_count -= 1;
		TUI.x -= 1;
		update = yes;
	}

	if( input_pressed( keyboard_tab ) )
	{
		n1 word_start = TUI.input_bytes_count;
		while( word_start > 0 and TUI.input_bytes[ word_start - 1 ] isnt ' ' )
		{
			word_start--;
		}
		n1 const prefix_len = TUI.input_bytes_count - word_start;

		if( prefix_len > 0 )
		{
			iter_inv( button_id, TUI.button_count )
			{
				if( _TUI_button_match( button_id, ref_of( TUI.input_bytes[ word_start ] ), prefix_len ) )
				{
					TUI_button const ref const button_ref = ref_of( TUI.buttons[ button_id ] );
					n2 const row_start = TUI.row_start[ button_ref->y ];
					n2 const row_end = TUI.row_start[ button_ref->y + 1 ];
					n2 const button_start = button_ref->x + TUI.input_bytes_count;
					n2 const button_end = button_ref->x + button_ref->size;

					range( byte_id, row_start, row_end - 1 )
					{
						TUI_byte const this_byte = TUI.bytes[ byte_id ];
						if( this_byte.x >= button_start and this_byte.x < button_end )
						{
							_TUI_print_byte( this_byte.byte );
							TUI.input_bytes[ TUI.input_bytes_count++ ] = this_byte.byte;
						}
					}
					TUI.input_bytes[ TUI.input_bytes_count ] = eof_byte;
					update = yes;

					skip;
				}
			}
		}
	}

	if( input_pressed( keyboard_enter ) )
	{
		n1 start_id = 0;
		while( start_id < TUI.input_bytes_count and TUI.input_bytes[ start_id ] is ' ' )
		{
			start_id++;
		}

		if( start_id < TUI.input_bytes_count )
		{
			n1 end_id = TUI.input_bytes_count;
			while( end_id > start_id and TUI.input_bytes[ end_id - 1 ] is ' ' )
			{
				end_id--;
			}

			_TUI_execute_word( ref_of( TUI.input_bytes[ start_id ] ), end_id - start_id );

			if( TUI.reprint is yes )
			{
				_TUI_call_reprint();
			}
		}

		TUI.input_bytes_count = 0;
		TUI.input_bytes[ 0 ] = eof_byte;
		update = yes;
	}

	//

	if( TUI.reprint is yes )
	{
		_TUI_call_reprint();
		TUI.input_bytes_count = 0;
		update = yes;
	}

	if( update )
	{
		TUI.view_ref->update = yes;

		if( program.fps <= 0 )
		{
			window_ref_update( window_ref );
		}
	}
}

#pragma endregion

#pragma endregion hidden

////////////////////////////////
#pragma region | TUI / visible

////////////////
#pragma region | - api

#define TUI_clear _TUI_clear

fn TUI_reprint()
{
	TUI.reprint = yes;
}

fn TUI_update_now()
{
	out_if_nothing( TUI.window_ref );

	TUI.view_ref->update = yes;
	window_ref_update_now( TUI.window_ref );
}

#define TUI_print( BYTES... ) _TUI_print( DEFAULT( newline, BYTES ) )
#define TUI_newline() TUI_print()
#define TUI_print_button( BYTES, CALLBACK, DATA... ) _TUI_print_button( BYTES, CALLBACK, to( anon ref, DEFAULT( nothing, DATA ) ) )

fn TUI_click_buttons( byte const ref const ref const inputs, i4 const inputs_count )
{
	if( inputs_count > 1 )
	{
		range( input_id, 0, inputs_count - 1 )
		{
			byte const ref const arg = inputs[ input_id ];
			n1 len = 0;
			while( arg[ len ] isnt eof_byte and len < 255 )
			{
				len++;
			}

			bytes_paste( TUI.input_bytes, arg );
			TUI.input_bytes_count = len;
			skip_if( _TUI_execute_word( arg, len ) is no );

			if( TUI.reprint is yes )
			{
				_TUI_call_reprint();
			}
		}
	}
}

embed out_state TUI_command( byte const ref const command, flag const detach )
{
	_program_process_events();
	out_state const state = pick( program.cli, command( command ), command_silent( command, detach ) );
	out state;
}
#define TUI_command( COMMAND, DETACH... ) TUI_command( COMMAND, DEFAULT( no, DETACH ) )

#pragma endregion

#pragma endregion visible

#pragma endregion TUI

#pragma endregion definitions

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma region - START
//

fn _TUI_start( byte const ref const window_name, n2x2 const window_size, n2 const scale, fn_ref( anon, fn_print ), n2 const fps, byte const ref const ref _start_inputs, i4 _start_inputs_count )
{
	TUI.hover_target = TUI_max_buttons - 1;
	TUI.current_hover = TUI_max_buttons - 1;
	TUI.fore = TUI_color_white;
	TUI.fn_print = fn_print;
	TUI.reprint = yes;

	program.fps = fps;

	if( _start_inputs_count > 0 and bytes_match( _start_inputs[ 0 ], "cli\0" ) )
	{
		program.cli = yes;

		_start_inputs += 1; // skip the input
		_start_inputs_count -= 1;

		TUI.window_ref = nothing;

		#if OS_WINDOWS
			AttachConsole( ATTACH_PARENT_PROCESS );
			freopen( "CONOUT$", "w", stdout );
			freopen( "CONOUT$", "w", stderr );
			freopen( "CONIN$", "r", stdin );
		#endif

		jump process_inputs;
	}
	else
	{
		program.cli = no;
		TUI.window_ref = program_make_window_ref( window_name, window_size, nothing, _TUI_window_tick );
	}

	pep deserial_pep = pep_deserialize( to( n1 const ref const, font_pep ), size_of( font_pep ) );
	TUI.font_main = make_font( make_canvas( deserial_pep.width, deserial_pep.height, to( pixel ref, pep_decompress( ref_of( deserial_pep ), pep_bgra, yes, no ) ) ) );
	pep_free( ref_of( deserial_pep ) );

	TUI.view_ref = window_ref_make_view_ref( TUI.window_ref, make_canvas( TUI_max_line_size * TUI.font_main.letter_size.w, TUI_max_lines * TUI.font_main.letter_size.h ), _TUI_view_draw );
	_TUI_set_scale( r4( scale ) );
	TUI.view_ref->clear = yes;

	//

	process_inputs:
	if( _start_inputs_count > 0 )
	{
		call( TUI.fn_print );
		TUI_click_buttons( _start_inputs, _start_inputs_count );
	}

	if( program.cli is yes )
	{
		#if OS_WINDOWS
			fflush( stdout );
			fflush( stderr );

			INPUT_RECORD ir[ 2 ] = { 0 };
			ir[ 0 ].EventType = ir[ 1 ].EventType = KEY_EVENT;
			ir[ 0 ].Event.KeyEvent.wRepeatCount = ir[ 1 ].Event.KeyEvent.wRepeatCount = 1;
			ir[ 0 ].Event.KeyEvent.wVirtualKeyCode = ir[ 1 ].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
			ir[ 0 ].Event.KeyEvent.uChar.AsciiChar = ir[ 1 ].Event.KeyEvent.uChar.AsciiChar = '\r';
			ir[ 0 ].Event.KeyEvent.bKeyDown = TRUE;
			DWORD w;
			WriteConsoleInputA( GetStdHandle( STD_INPUT_HANDLE ), ir, 2, ref_of( w ) );

			FreeConsole();
		#endif
		exit( success );
	}
}
#define TUI_start( WINDOW_NAME, WINDOW_SIZE, VIEW_SCALE, FN_PRINT, FPS... ) _TUI_start( WINDOW_NAME, WINDOW_SIZE, VIEW_SCALE, FN_PRINT, DEFAULT( 0, FPS ), start_inputs + 1, start_inputs_count - 1 )

#pragma endregion start

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
