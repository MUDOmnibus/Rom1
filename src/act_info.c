/***************************************************************************
 *  file: act_info.c , Implementation of commands.         Part of DIKUMUD *
 *  Usage : Informative commands.                                          *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as this   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
extern int fseek();

#include "structs.h"
#include "mob.h"
#include "obj.h"
#include "utils.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"

/* extern variables */

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern char credits[MAX_STRING_LENGTH];
extern char news[MAX_STRING_LENGTH];
extern char motd[MAX_STRING_LENGTH];
extern char imotd[MAX_STRING_LENGTH];
extern char info[MAX_STRING_LENGTH];
extern char story[MAX_STRING_LENGTH];
extern char wizlist[MAX_STRING_LENGTH];
extern char *dirs[]; 
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern struct str_app_type str_app[];

/* extern functions */

struct time_info_data age(struct char_data *ch);
void page_string(struct descriptor_data *d, char *str, int keep_internal);

/* RT board functions */

int find_board (struct char_data *ch);
int Board_show_board(int board_type, struct char_data *ch);
int Board_display_msg(int board_type, struct char_data *ch, char *arg);
int Board_remove_msg(int board_type, struct char_data *ch, char *arg);
int Board_save_board(int board_type);

/* intern functions */

void list_obj_to_char(struct obj_data *list,struct char_data *ch, int mode,
    bool show);

/* RT board erase call */

void do_erase(struct char_data *ch, char *argument, int cmd)
{
  char temp[MAX_STRING_LENGTH];
  struct obj_data *obj_board;
  int board_type;

  obj_board = get_obj_in_list_vis(ch,"board",world[ch->in_room].contents);
  if (!obj_board)
  {
    send_to_char("You do not see a board here.\n\r",ch);
    return;
  }

  one_argument(argument,temp);
  if (!is_number(temp))
  {
    send_to_char("You must provide a number to erase.\n\r",ch);
    return;
  }

  board_type = find_board(ch);
  if (board_type != -1)
  {
    Board_remove_msg(board_type,ch,argument);
    Board_save_board(board_type);
  }
  else
  {
    send_to_char("You do not see a board here.\n\r",ch);
    return;
  }
}


/* Procedures related to 'look' */

void argument_split_2(char *argument, char *first_arg, char *second_arg) {
    int look_at, found, begin;
    found = begin = 0;

    /* Find first non blank */
    for ( ;*(argument + begin ) == ' ' ; begin++);

    /* Find length of first word */
    for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)

    /* Make all letters lower case, AND copy them to first_arg */
    *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;

    /* Find first non blank */
    for ( ;*(argument + begin ) == ' ' ; begin++);

    /* Find length of second word */
    for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

    /* Make all letters lower case, AND copy them to second_arg */
    *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(second_arg + look_at)='\0';
    begin += look_at;
}

struct obj_data *get_object_in_equip_vis(struct char_data *ch,
    char *arg, struct obj_data *equipment[], int *j) {

    for ((*j) = 0; (*j) < MAX_WEAR ; (*j)++)
	if (equipment[(*j)])
	    if (CAN_SEE_OBJ(ch,equipment[(*j)]))
		if (isname(arg, equipment[(*j)]->name))
		    return(equipment[(*j)]);

    return (0);
}

char *find_ex_description(char *word, struct extra_descr_data *list)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
	if (isname(word,i->keyword))
	    return(i->description);

    return(0);
}

void show_obj_to_char(struct obj_data *object, struct char_data *ch, int mode)
{
    char buffer[SHORT_STRING_LENGTH];
    bool found;

    buffer[0] = '\0';
    if ((mode == 0) && object->description)
	strcpy(buffer,object->description);
    else    if (object->short_description && ((mode == 1) ||
	  (mode == 2) || (mode==3) || (mode == 4))) 
	strcpy(buffer,object->short_description);
    else if (mode == 5) {
	if (object->obj_flags.type_flag == ITEM_NOTE)
	{
	    if (object->action_description)
	    {
		strcpy(buffer, "There is something written upon it:\n\r\n\r");
		strcat(buffer, object->action_description);
		page_string(ch->desc, buffer, 1);
	    }
	    else
		act("It's blank.", FALSE, ch,0,0,TO_CHAR);
	    return;
	}
	else if((object->obj_flags.type_flag != ITEM_DRINKCON))
	{
	    strcpy(buffer,"You see nothing special.");
	}
	else /* ITEM_TYPE == ITEM_DRINKCON */
	{
	    strcpy(buffer, "It looks like a drink container.");
	}
    }

    if (mode != 3) { 
	found = FALSE;
	if (IS_OBJ_STAT(object,ITEM_INVISIBLE)) {
	     strcat(buffer,"(invisible)");
	     found = TRUE;
	}
	if (IS_OBJ_STAT(object,ITEM_EVIL) && IS_AFFECTED(ch,AFF_DETECT_EVIL)) {
	     strcat(buffer,"..It glows red!");
	     found = TRUE;
	}
	if (IS_OBJ_STAT(object,ITEM_MAGIC) && IS_AFFECTED(ch,AFF_DETECT_MAGIC)) {
	     strcat(buffer,"..It glows blue!");
	     found = TRUE;
	}
	if (IS_OBJ_STAT(object,ITEM_GLOW)) {
	    strcat(buffer,"..It has a soft glowing aura!");
	    found = TRUE;
	}
	if (IS_OBJ_STAT(object,ITEM_HUM)) {
	    strcat(buffer,"..It emits a faint humming sound!");
	    found = TRUE;
	}
    }

    strcat(buffer, "\n\r");
    page_string(ch->desc, buffer, 1);
}

void list_obj_to_char(struct obj_data *list,struct char_data *ch, int mode, 
    bool show) {
    struct obj_data *i;
    bool found;

    found = FALSE;
    for ( i = list ; i ; i = i->next_content ) { 
	if (CAN_SEE_OBJ(ch,i)) {
	    show_obj_to_char(i, ch, mode);
	    found = TRUE;
	}    
    }  
    if ((! found) && (show)) send_to_char("Nothing\n\r", ch);
}



void show_char_to_char(struct char_data *i, struct char_data *ch, int mode)
{
    char buffer[SHORT_STRING_LENGTH];
    int j, found, percent;
    struct obj_data *tmp_obj;

    if (mode == 0) {

	if (IS_AFFECTED(i, AFF_HIDE) || !CAN_SEE(ch,i)) {
	    if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
		send_to_char("You sense a hidden life form in the room.\n\r", ch);
	    return;
	}

	if (!(i->player.long_descr)||(GET_POS(i) != i->specials.default_pos)){
	/* A char without long descr, or not in default pos. */
	    if (!IS_NPC(i)) {   
		strcpy(buffer,GET_NAME(i));
		if ( !IS_SET(ch->specials.act, PLR_BRIEF) )
		{
		    strcat(buffer," ");
		    strcat(buffer,GET_TITLE(i));
		}
	    } else {
		strcpy(buffer, i->player.short_descr);
		(void) CAP(buffer);
	    }

	    if ( IS_AFFECTED(i,AFF_INVISIBLE))
	       strcat(buffer," (invisible)");

	    switch(GET_POS(i)) {
		case POSITION_STUNNED  : 
		    strcat(buffer," is lying here, stunned."); break;
		case POSITION_INCAP    : 
		    strcat(buffer," is lying here, incapacitated."); break;
		case POSITION_MORTALLYW: 
		    strcat(buffer," is lying here, mortally wounded."); break;
		case POSITION_DEAD     : 
		    strcat(buffer," is lying here, dead."); break;
		case POSITION_STANDING : 
		    strcat(buffer," is here."); break;
		case POSITION_SITTING  : 
		    strcat(buffer," is sitting here.");  break;
		case POSITION_RESTING  : 
		    strcat(buffer," is resting here.");  break;
		case POSITION_SLEEPING : 
		    strcat(buffer," is sleeping here."); break;
		case POSITION_FIGHTING :
		    if (i->specials.fighting) {

			strcat(buffer," is here, fighting ");
			if (i->specials.fighting == ch)
			    strcat(buffer," YOU!");
			else {
			    if (i->in_room == i->specials.fighting->in_room)
				if (IS_NPC(i->specials.fighting))
				    strcat(buffer, i->specials.fighting->player.short_descr);
				else
				    strcat(buffer, GET_NAME(i->specials.fighting));
			    else
				strcat(buffer, "someone who has already left.");
			}
		    } else /* NIL fighting pointer */
			    strcat(buffer," is here struggling with thin air.");
		    break;
		default : strcat(buffer," is floating here."); break;
	    }
	    if (IS_AFFECTED(ch, AFF_DETECT_EVIL)) {
		if (IS_EVIL(i))
		    strcat(buffer, " (Red Aura)");
	    }

	    strcat(buffer,"\n\r");
	    send_to_char(buffer, ch);
	}
	else  /* npc with long */
	{
	    if (IS_AFFECTED(i,AFF_INVISIBLE))
		strcpy(buffer,"*");
	    else
		*buffer = '\0';

	    if (IS_AFFECTED(ch, AFF_DETECT_EVIL)) {
		if (IS_EVIL(i))
		    strcat(buffer, " (Red Aura)");
	    }

	    strcat(buffer, i->player.long_descr);

	    send_to_char(buffer, ch);
	}
			    
	if (IS_AFFECTED(i,AFF_SANCTUARY))
	    act("$n glows with a bright light!", FALSE, i, 0, ch, TO_VICT);

    } else if (mode == 1) {

	if (i->player.description)
	    send_to_char(i->player.description, ch);
	else {
	    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);
	}

	/* Show a character to another */

	if (GET_MAX_HIT(i) > 0)
	    percent = (100*GET_HIT(i))/GET_MAX_HIT(i);
	else
	    percent = -1; /* How could MAX_HIT be < 1?? */

	if (IS_NPC(i))
	    strcpy(buffer, i->player.short_descr);
	else
	    strcpy(buffer, GET_NAME(i));

	if (percent >= 100)
	    strcat(buffer, " is in an excellent condition.\n\r");
	else if (percent >= 90)
	    strcat(buffer, " has a few scratches.\n\r");
	else if (percent >= 75)
	    strcat(buffer, " has some small wounds and bruises.\n\r");
	else if (percent >= 50)
	    strcat(buffer, " has quite a few wounds.\n\r");
	else if (percent >= 30)
	    strcat(buffer, " has some big nasty wounds and scratches.\n\r");
	else if (percent >= 15)
	    strcat(buffer, " looks pretty hurt.\n\r");
	else if (percent >= 0)
	    strcat(buffer, " is in an awful condition.\n\r");
	else
	    strcat(buffer, " is bleeding awfully from big wounds.\n\r");

	send_to_char(buffer, ch);

	found = FALSE;
	for (j=0; j< MAX_WEAR; j++) {
	    if (i->equipment[j]) {
		if (CAN_SEE_OBJ(ch,i->equipment[j])) {
		    found = TRUE;
		}
	    }
	}
	if (found) {
	    act("\n\r$n is using:", FALSE, i, 0, ch, TO_VICT);
	    for (j=0; j< MAX_WEAR; j++) {
		if (i->equipment[j]) {
		    if (CAN_SEE_OBJ(ch,i->equipment[j])) {
			send_to_char(where[j],ch);
			show_obj_to_char(i->equipment[j],ch,1);
		    }
		}
	    }
	}
	if ((GET_CLASS(ch) == CLASS_THIEF && ch != i) || GET_LEVEL(ch)>31){
	    found = FALSE;
	    send_to_char("\n\rYou attempt to peek at the inventory:\n\r", ch);
	    for(tmp_obj = i->carrying; tmp_obj;
		tmp_obj = tmp_obj->next_content) {
		if (CAN_SEE_OBJ(ch, tmp_obj) && number(0,20) < GET_LEVEL(ch)) {
		    show_obj_to_char(tmp_obj, ch, 1);
		    found = TRUE;
		}
	    }
	    if (!found)
		send_to_char("You can't see anything.\n\r", ch);
	}

    } else if (mode == 2) {

	/* Lists inventory */
	act("$n is carrying:", FALSE, i, 0, ch, TO_VICT);
	list_obj_to_char(i->carrying,ch,1,TRUE);
    }
}



void list_char_to_char(struct char_data *list, struct char_data *ch, 
    int mode) {
    struct char_data *i;

    for (i = list; i ; i = i->next_in_room) {
	if ( (ch!=i) && ((IS_AFFECTED(ch, AFF_SENSE_LIFE)
	     && !i->specials.wizInvis) ||
	     (CAN_SEE(ch,i) && !IS_AFFECTED(i, AFF_HIDE))) ){
	    show_char_to_char(i,ch,0);
	      } else {
	    if ((IS_DARK(ch->in_room)) && (IS_AFFECTED(i, AFF_INFRARED))){
	      /* Monster with infra red : can't see him */
	      send_to_char(
	"You see a pair of glowing red eyes looking your way.\n\r",ch);
	    }
      }
    } 
}


/* RT code for changing the size of the scroll buffer */
void do_scroll(struct char_data *ch, char *argument, int cmd)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int new_rows;

    one_argument(argument,arg);
    if (!*arg)
    {
      sprintf(buf,"You currently display %d lines.\n\r",ch->desc->rows);
      send_to_char(buf,ch);
      return;
    }
    new_rows = atoi(arg);
    if (!new_rows || new_rows < 20)
    {
      send_to_char("You must provide a number greater than 20.\n\r",ch);
      return;
    }
    ch->desc->rows = new_rows;
    ch->specials.rows = new_rows;
    sprintf(buf,"You now will display %d lines.\n\r",ch->desc->rows);
    send_to_char(buf,ch);
}

void do_look(struct char_data *ch, char *argument, int cmd)
{
    char buffer[SHORT_STRING_LENGTH];
    char output[SHORT_STRING_LENGTH];
    char arg1[SHORT_STRING_LENGTH];
    char arg2[SHORT_STRING_LENGTH];
    int keyword_no;
    int door;
    char *exits[] =
    {
        "North",
        "East",
        "South",
        "West",
        "Up",
        "Down"
    };
    int j, bits, temp;
    bool found;
    struct obj_data *tmp_object, *found_object;
    struct char_data *tmp_char;
    char *tmp_desc;
    static char *keywords[]= { 
	"north",
	"east",
	"south",
	"west",
	"up",
	"down",
	"in",
	"at",
	"",  /* Look at '' case */
	"\n" };

    if (!ch->desc)
	return;

    if (GET_POS(ch) < POSITION_SLEEPING)
	send_to_char("You can't see anything but stars!\n\r", ch);
    else if (GET_POS(ch) == POSITION_SLEEPING)
	send_to_char("You can't see anything, you're sleeping!\n\r", ch);
    else if ( check_blind(ch) )
	;
    else if ( IS_DARK(ch->in_room) && !ch->specials.holyLite){
	send_to_char("It is pitch black...\n\r", ch);
	list_char_to_char(world[ch->in_room].people, ch, 0);
    } else {
	argument_split_2(argument,arg1,arg2);
	keyword_no = search_block(arg1, keywords, FALSE); /* Partiel Match */

	if ((keyword_no == -1) && *arg1) {
	    keyword_no = 7;
	    strcpy(arg2, arg1); /* Let arg2 become the target object (arg1) */
	}

	found = FALSE;
	tmp_object = 0;
	tmp_char     = 0;
	tmp_desc     = 0;

	switch(keyword_no) {
	    /* look <dir> */
	    case 0 :
	    case 1 :
	    case 2 : 
	    case 3 : 
	    case 4 :
	    case 5 : {   

		if (EXIT(ch, keyword_no)) {

		    if (EXIT(ch, keyword_no)->general_description) {
			send_to_char(EXIT(ch, keyword_no)->
			    general_description, ch);
		    } else {
			send_to_char("You see nothing special.\n\r", ch);
		    }

		    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) && 
			(EXIT(ch, keyword_no)->keyword)) {
			    sprintf(buffer, "The %s is closed.\n\r",
				fname(EXIT(ch, keyword_no)->keyword));
			    send_to_char(buffer, ch);
		    }   else {
			if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR)
			 && EXIT(ch, keyword_no)->keyword) {
			    sprintf(buffer, "The %s is open.\n\r",
				fname(EXIT(ch, keyword_no)->keyword));
			    send_to_char(buffer, ch);
			}
		    }
		} else {
			send_to_char("Nothing special there...\n\r", ch);
		}
	    }
	    break;

	    /* look 'in'    */
	    case 6: {
		if (*arg2) {
		    /* Item carried */

		    bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
			     FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

		    if (bits) { /* Found something */
			if (GET_ITEM_TYPE(tmp_object)== ITEM_DRINKCON)
			{
			    if (tmp_object->obj_flags.value[1] <= 0) {
				act("It is empty.", FALSE, ch, 0, 0, TO_CHAR);
			    } else {
				temp=((tmp_object->obj_flags.value[1]*3)
					/tmp_object->obj_flags.value[0]);
				sprintf(buffer,
					"It's %sfull of a %s liquid.\n\r",
					fullness[temp],
					color_liquid[tmp_object->
						obj_flags.value[2]]);
				send_to_char(buffer, ch);
			    }
			} else if (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER)
			 {
			    if (!IS_SET(tmp_object->obj_flags.value[1],
					CONT_CLOSED)) {
				send_to_char(fname(tmp_object->name), ch);
				switch (bits) {
				    case FIND_OBJ_INV :
					send_to_char(" (carried) : \n\r", ch);
					break;
				    case FIND_OBJ_ROOM :
					send_to_char(" (here) : \n\r", ch);
					break;
				    case FIND_OBJ_EQUIP :
					send_to_char(" (used) : \n\r", ch);
					break;
				}
				list_obj_to_char(tmp_object->contains,
					 ch, 2, TRUE);
			    }
			    else
				send_to_char("It is closed.\n\r", ch);
			} else {
			    send_to_char("That is not a container.\n\r", ch);
			}
		    } else { /* wrong argument */
			send_to_char("You do not see that item here.\n\r", ch);
		    }
		} else { /* no argument */
		    send_to_char("Look in what?!\n\r", ch);
		}
	    }
	    break;

	    /* look 'at'    */
	    case 7 : {


		if (*arg2) {

		    bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
			   FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
			   ch, &tmp_char, &found_object);

		    if (tmp_char) {
			show_char_to_char(tmp_char, ch, 1);
			if (ch != tmp_char) {
			  if (GET_LEVEL(ch)>31 && ch->specials.wizInvis
				    == TRUE) {
			    return;
			  }
			  act("$n looks at you.",
				TRUE, ch, 0, tmp_char, TO_VICT);
			  act("$n looks at $N.",
				TRUE, ch, 0, tmp_char, TO_NOTVICT);
			}
			return;
		    }


		    /* Search for Extra Descriptions in room and items */

		    /* Extra description in room?? */
		    if (!found) {
			tmp_desc = find_ex_description(arg2, 
			    world[ch->in_room].ex_description);
			if (tmp_desc) {
			    page_string(ch->desc, tmp_desc, 0);
			    return; /* RETURN SINCE IT WAS ROOM DESCRIPTION */
			    /* Old system was: found = TRUE; */
			}
		    }

		    /* Search for extra descriptions in items */

		    /* Equipment Used */

		    if (!found) {
			for (j = 0; j< MAX_WEAR && !found; j++) {
			    if (ch->equipment[j]) {
				if (CAN_SEE_OBJ(ch,ch->equipment[j])) {
				    tmp_desc = find_ex_description(arg2, 
					ch->equipment[j]->ex_description);
				    if (tmp_desc) {
					page_string(ch->desc, tmp_desc, 1);
					found = TRUE;
				    }
				}
			    }
			}
		    }

		    /* In inventory */

		    if (!found) {
			for(tmp_object = ch->carrying; 
			    tmp_object && !found; 
			    tmp_object = tmp_object->next_content) {
			    if (CAN_SEE_OBJ(ch, tmp_object)) {
				tmp_desc = find_ex_description(arg2, 
				    tmp_object->ex_description);
				if (tmp_desc) {
				    page_string(ch->desc, tmp_desc, 1);
				    found = TRUE;
				}
			    }
			}
		    }

		    /* Object In room */

		    if (!found) {
			for(tmp_object = world[ch->in_room].contents; 
			    tmp_object && !found; 
			    tmp_object = tmp_object->next_content) {
			    if (CAN_SEE_OBJ(ch, tmp_object)) {
				tmp_desc = find_ex_description(arg2, 
				    tmp_object->ex_description);
/* RT for looking at a board */
                                if (tmp_object->obj_flags.type_flag ==
                                    ITEM_BOARD)
 
                                {
                                  temp = find_board(ch);
                                  if (temp != -1)
				    Board_show_board(temp,ch);
				  found = TRUE;
				}
				else if (tmp_desc) {
				    page_string(ch->desc, tmp_desc, 1);
				    found = TRUE;
				}
			    }
			}
		    }
		    /* wrong argument */

		    if (bits) { /* If an object was found */
			if (!found)
			    /* Show no-description */
			    show_obj_to_char(found_object, ch, 5);
			else
			    /* Find hum, glow etc */
			    show_obj_to_char(found_object, ch, 6);
		    } else if (!found) {
			send_to_char("You do not see that here.\n\r", ch);
		    }
		} else {
		    /* no argument */

		    send_to_char("Look at what?\n\r", ch);
		}
	    }
	    break;


	    /* look ''      */ 
	    case 8 : {

		send_to_char(world[ch->in_room].name, ch);
		send_to_char("\n\r", ch);

		if (!IS_SET(ch->specials.act, PLR_BRIEF))
		    send_to_char(world[ch->in_room].description, ch);
		if (IS_SET(ch->specials.act, PLR_AUTOEXIT))
		{
		  *buffer = '\0';
		  if (!check_blind(ch))
		  {
		    for (door = 0; door <= 5; door ++)
		    {
		      if (EXIT(ch, door))
		 	if (EXIT(ch,door)->to_room != NOWHERE &&
			    !IS_SET(EXIT(ch,door)->exit_info,EX_CLOSED))
			  sprintf(buffer + strlen(buffer), "%s ", exits[door]);
                    }
                  }
		  if (!*buffer)	
 		    sprintf(buffer,"None");
 	   	  sprintf(output,"\n\rExits: %s\n\r", buffer);
		  send_to_char(output,ch);
                }

		list_obj_to_char(world[ch->in_room].contents, ch, 0,FALSE);

		list_char_to_char(world[ch->in_room].people, ch, 0);
	    }
	    break;

	    /* wrong arg    */
	    case -1 : 
		send_to_char("Sorry, I didn't understand that!\n\r", ch);
		break;
	}
    }
}

/* end of look */




void do_read(struct char_data *ch, char *argument, int cmd)
{
    char buf[100];
    int num, board_type;
    struct obj_data *board;

    /* RT code for read <number> */
   
    one_argument(argument,buf); 
    if (is_number(buf))
    {
      num = atoi(buf);
      board = get_obj_in_list_vis(ch,"board",world[ch->in_room].contents);
      if (!board)
      {
 	send_to_char("You do not see a board here.\n\r",ch);
	return;
      }
      board_type = find_board(ch);
      if (board_type != -1)
        Board_display_msg(board_type,ch,argument);
      else
      {
	send_to_char("You do not see a board here.\n\r",ch);
	return;
      }
      return;
    } 

    /* This is just for now - To be changed later.! */
    sprintf(buf,"at %s",argument);
    do_look(ch,buf,15);
}



void do_examine(struct char_data *ch, char *argument, int cmd)
{
    char name[100], buf[100];
    int bits;
    struct char_data *tmp_char;
    struct obj_data *tmp_object;

    sprintf(buf,"at %s",argument);
    do_look(ch,buf,15);

    one_argument(argument, name);

    if (!*name)
    {
	send_to_char("Examine what?\n\r", ch);
	return;
    }

    bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM |
	   FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

    if (tmp_object) {
	if ((GET_ITEM_TYPE(tmp_object)==ITEM_DRINKCON) ||
	    (GET_ITEM_TYPE(tmp_object)==ITEM_CONTAINER)) {
	    send_to_char("When you look inside, you see:\n\r", ch);
	    sprintf(buf,"in %s",argument);
	    do_look(ch,buf,15);
	}
    }
}



void do_exits(struct char_data *ch, char *argument, int cmd)
{
    int door;
    char buf[SHORT_STRING_LENGTH];
    char *exits[] =
    {   
	"North",
	"East ",
	"South",
	"West ",
	"Up   ",
	"Down "
    };

    *buf = '\0';

    if ( check_blind(ch) )
	return;

    for (door = 0; door <= 5; door++)
	if (EXIT(ch, door))
	    if (EXIT(ch, door)->to_room != NOWHERE &&
		!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
		if (IS_DARK(EXIT(ch, door)->to_room) && !ch->specials.holyLite)
		    sprintf(buf + strlen(buf), "%s - Too dark to tell\n\r",
			exits[door]);
		else
		    sprintf(buf + strlen(buf), "%s - %s\n\r", exits[door],
			world[EXIT(ch, door)->to_room].name);

    send_to_char("Obvious exits:\n\r", ch);

    if (*buf)
	send_to_char(buf, ch);
    else
	send_to_char("None.\n\r", ch);
}


void do_score(struct char_data *ch, char *argument, int cmd)
{
    char buf[SHORT_STRING_LENGTH];
    struct affected_type *aff;
	extern char *apply_types[];
	extern char *spells[];


    sprintf(buf, "You are %d years old.", GET_AGE(ch));

    if ((age(ch).month == 0) && (age(ch).day == 0))
	strcat(buf," It's your birthday today.\n\r");
    else
	strcat(buf,"\n\r");
    send_to_char(buf, ch);

    if (GET_COND(ch,DRUNK)>10)
	send_to_char("You are intoxicated.\n\r", ch);
    if (!GET_COND(ch,THIRST))
	send_to_char("You are thirsty.\n\r", ch);
    if (!GET_COND(ch,FULL))
	send_to_char("You are hungry.\n\r", ch);

    sprintf( buf, 
    "You have %d(%d) hit, %d(%d) mana, %d(%d) movement, %d practices.\n\r",
	GET_HIT(ch),  GET_MAX_HIT(ch),
	GET_MANA(ch), GET_MAX_MANA(ch),
	GET_MOVE(ch), GET_MAX_MOVE(ch),
	ch->specials.practices );
    send_to_char(buf,ch);


    sprintf(buf,"Str: %d  Int: %d  Wis: %d  Dex: %d  Con: %d.\n\r",
	GET_STR(ch),
	GET_INT(ch),GET_WIS(ch),
	GET_DEX(ch),GET_CON(ch));
    send_to_char(buf,ch);

    sprintf(buf,"You have scored %d exp, and have %d gold coins.\n\r",
	GET_EXP(ch),GET_GOLD(ch));
    send_to_char(buf,ch);
    if (GET_LEVEL(ch)<31){
      sprintf(buf,"You need %d exps to level %d.\n\r",
	  exp_table[GET_LEVEL(ch)+1] - GET_EXP(ch), GET_LEVEL(ch) + 1 );
      send_to_char(buf,ch);
    } else {
      sprintf(buf,
	"You are Immortal.  You do not need any more exps to level.\n\r");
      send_to_char(buf,ch);
    }


    sprintf( buf, "You are carrying %d/%d items with weight %d/%d pounds.\n\r",
	IS_CARRYING_N(ch), CAN_CARRY_N(ch),
	IS_CARRYING_W(ch), CAN_CARRY_W(ch) );
    send_to_char( buf, ch );


    send_to_char("Armor:  ",ch);
    if (GET_LEVEL(ch)>=20)
      {
	sprintf(buf, "%d.\n\r", GET_AC(ch));
	send_to_char(buf, ch);
      }
    
	if ((GET_AC(ch)<101) && (GET_AC(ch)>90))
	  send_to_char("You are naked.  Better get some clothes.\n\r",ch);
	else
	if ((GET_AC(ch)<91) && (GET_AC(ch)>70))
	  send_to_char("At least you are wearing clothes.\n\r",ch);
	else
	if ((GET_AC(ch)<71) && (GET_AC(ch)>50))
	  send_to_char("You feel slightly armored.\n\r",ch);
	else
	if ((GET_AC(ch)<51) && (GET_AC(ch)>40))
	  send_to_char("You feel partially armored.\n\r",ch);
	else
	if ((GET_AC(ch)<41) && (GET_AC(ch)>20))
	  send_to_char("You feel armored.\n\r",ch);
	else
	if ((GET_AC(ch)<21) && (GET_AC(ch)>10))
	  send_to_char("You feel heavily armored.\n\r",ch);
	else
	    if ((GET_AC(ch)<11) && (GET_AC(ch)>-10))
	  send_to_char("You are very heavily armored.\n\r",ch);
	else
	    if ((GET_AC(ch)<-9) && (GET_AC(ch)>-30))
	  send_to_char("You are superbly armored.\n\r",ch);
	else
	if ((GET_AC(ch)<-29) && (GET_AC(ch)>-50))
	  send_to_char("Your entire body is covered with armor.\n\r",ch);
	else
	if ((GET_AC(ch)<-49) && (GET_AC(ch)>-70))
	  send_to_char("You feel invincible!\n\r",ch);
	else
	    if ((GET_AC(ch)<-69) && (GET_AC(ch)>-90))
	  send_to_char("You have the gods on your side.\n\r",ch);
	else
	if ((GET_AC(ch)<-89) && (GET_AC(ch)>-120))
	  send_to_char("You are wearing divine armor.  May I have some?\n\r",
		ch);
	else
	if ((GET_AC(ch)<-119) && (GET_AC(ch)>-140))
	  send_to_char("You are a walking juggernaut.\n\r",ch);
	else
	if (GET_AC(ch)<-139)
	  send_to_char("Nothing can touch you now!\n\r",ch);

    if (GET_LEVEL(ch)>=15){
      sprintf(buf,"Hitroll: %d  Damroll: %d.\n\r",
	  GET_HITROLL(ch),
	  GET_DAMROLL(ch));
      send_to_char(buf,ch);
    }
    
    if ((GET_ALIGNMENT(ch)<1001) && (GET_ALIGNMENT(ch)>900))
      send_to_char("You are a saint.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<901) && (GET_ALIGNMENT(ch)>700))
	send_to_char("Goodness exudes from your body.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<701) && (GET_ALIGNMENT(ch)>500))
	send_to_char("You are very good.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<501) && (GET_ALIGNMENT(ch)>350))
	send_to_char("You are good.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<351) && (GET_ALIGNMENT(ch)>300))
	send_to_char("You are almost good.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<301) && (GET_ALIGNMENT(ch)>100))
	send_to_char("You are neutral with tendencies toward good.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<101) && (GET_ALIGNMENT(ch)>-101))
	send_to_char("You are neutral.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-100) && (GET_ALIGNMENT(ch)>-301))
	send_to_char("You are neutral with tendencies toward evil.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-300) && (GET_ALIGNMENT(ch)>-351))
	send_to_char("You are almost evil.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-350) && (GET_ALIGNMENT(ch)>-501))
	send_to_char("You are evil.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-500) && (GET_ALIGNMENT(ch)>-701))
	send_to_char("You are very evil.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-700) && (GET_ALIGNMENT(ch)>-901))
	send_to_char("Evil exudes from your body.\n\r",ch);
    else
      if ((GET_ALIGNMENT(ch)<-900) && (GET_ALIGNMENT(ch)>-1001))
	send_to_char("You are prime evil itself.\n\r",ch);
    
    sprintf( buf, "You have been playing for %d hours.\n\r",
        (ch->player.time.played + time(0) - ch->player.time.logon) / 3600 );
    send_to_char(buf, ch);      

    sprintf(buf,"This ranks you as %s %s (level %d).\n\r",
	GET_NAME(ch),
	GET_TITLE(ch), GET_LEVEL(ch));
    send_to_char(buf,ch);

    switch(GET_POS(ch)) {
	case POSITION_DEAD : 
	    send_to_char("You are DEAD!\n\r", ch); break;
	case POSITION_MORTALLYW :
	    send_to_char(
		"You are mortally wounded!, you should seek help!\n\r",
		ch); break;
	case POSITION_INCAP : 
	    send_to_char("You are incapacitated, slowly fading away.\n\r", ch);
	    break;
	case POSITION_STUNNED : 
	    send_to_char("You are stunned! You can't move\n\r", ch); break;
	case POSITION_SLEEPING : 
	    send_to_char("You are sleeping.\n\r",ch); break;
	case POSITION_RESTING  : 
	    send_to_char("You are resting.\n\r",ch); break;
	case POSITION_SITTING  : 
	    send_to_char("You are sitting.\n\r",ch); break;
	case POSITION_FIGHTING :
	    if (ch->specials.fighting)
		act("You are fighting $N.\n\r", FALSE, ch, 0,
		     ch->specials.fighting, TO_CHAR);
	    else
		send_to_char("You are fighting thin air.\n\r", ch);
	    break;
	case POSITION_STANDING : 
	    send_to_char("You are standing.\n\r",ch); break;
	default :
	    send_to_char("You are floating.\n\r",ch); break;
	      }
    if(IS_AFFECTED(ch,AFF_GROUP)){
      send_to_char("You are grouped.\n\r",ch);
    }

    if ((GET_LEVEL(ch) > 30) && (ch->specials.invis_level > 30))
    {
	sprintf(buf, "Invisible: level %d.\n\r",ch->specials.invis_level);
	send_to_char(buf,ch);
    }

    if (ch->affected)
    {
	send_to_char("You are affected by:\n\r",ch);
	for(aff = ch->affected; aff; aff = aff->next)
	{
	    sprintf(buf, "Spell : '%s'\n\r",spells[aff->type-1]);
	    send_to_char(buf, ch);
	    if (GET_LEVEL(ch)>19)
	    {
		sprintf( buf,
    "     Modifies %s by %d points and expires in %3d hours.\n\r",
		    apply_types[(int) aff->location],
		    aff->modifier,
		    aff->duration);
		send_to_char(buf, ch);
	    }
	}
    }
}


void do_time(struct char_data *ch, char *argument, int cmd)
{
    char buf[100], *suf;
    int weekday, day;
    long ct;
    extern struct time_info_data time_info;
    extern const char *weekdays[];
    extern const char *month_name[];

    /* 35 days in a month */
    weekday = ((35*time_info.month)+time_info.day+1) % 7;

    sprintf(buf, "It is %d o'clock %s, on %s.\n\r",
	((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	((time_info.hours >= 12) ? "pm" : "am"),
	weekdays[weekday]);

    send_to_char(buf,ch);

    day = time_info.day + 1;   /* day in [1..35] */

    if (day == 1)
	suf = "st";
    else if (day == 2)
	suf = "nd";
    else if (day == 3)
	suf = "rd";
    else if (day < 20)
	suf = "th";
    else if ((day % 10) == 1)
	suf = "st";
    else if ((day % 10) == 2)
	suf = "nd";
    else if ((day % 10) == 3)
	suf = "rd";
    else
	suf = "th";

    sprintf(buf, "The %d%s Day of the %s, Year %d.\n\r",
	day,
	suf,
	month_name[time_info.month],
	time_info.year);

    send_to_char(buf,ch);
    ct = time(0);
    sprintf( buf, "The system time is %s\r", ctime(&ct) );
}


void do_weather(struct char_data *ch, char *argument, int cmd)
{
    extern struct weather_data weather_info;
    char buf[256];
    static char *sky_look[4]= {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"};

    if (OUTSIDE(ch)) {
	sprintf(buf, 
	"The sky is %s and %s.\n\r",
	    sky_look[weather_info.sky],
	    (weather_info.change >=0 ? "you feel a warm wind from south" :
	 "your foot tells you bad weather is due"));
	send_to_char(buf,ch);
    } else
	send_to_char("You have no feeling about the weather at all.\n\r", ch);
}


void do_help(struct char_data *ch, char *argument, int cmd)
{
    extern int top_of_helpt;
    extern struct help_index_element *help_index;
    extern FILE *help_fl;
    extern char help[SHORT_STRING_LENGTH];

    int chk, bot, top, mid;
    char buf[SHORT_STRING_LENGTH], buffer[SHORT_STRING_LENGTH];


    if (!ch->desc)
	return;

    for(;isspace(*argument); argument++)  ;


    if (*argument)
    {
	if (!help_index)
	{
	    send_to_char("No help available.\n\r", ch);
	    return;
	}
	bot = 0;
	top = top_of_helpt;

	for (;;)
	{
	    mid = (bot + top) / 2;

	    if (!(chk = str_cmp(argument, help_index[mid].keyword)))
	    {
		fseek(help_fl, help_index[mid].pos, 0);
		*buffer = '\0';
		for (;;)
		{
		    fgets(buf, 80, help_fl);
		    if (*buf == '#')
			break;
		    strcat(buffer, buf);
		    strcat(buffer, "\r");
		}
		page_string(ch->desc, buffer, 1);
		return;
	    }
	    else if (bot >= top)
	    {
		send_to_char("There is no help on that word.\n\r", ch);
		return;
	    }
	    else if (chk > 0)
		bot = ++mid;
	    else
		top = --mid;
	}
	return;
    }


    send_to_char(help, ch);

}


void do_who(struct char_data *ch, char *argument, int cmd)
{
    struct descriptor_data *d;
    char output[MAX_STRING_LENGTH];
    char buf[256];
    char tmp[256];
    char temp[256];
    char arg1[SHORT_STRING_LENGTH];
    int  low_limit =0;
    int  high_limit = 0;
    int  check_class = -1;
    int  temp_limit = 0;
    int  count;
    int  found = 0;
   


    /* New parser -- kind of ugly, but not bad for not knowing C */
    count = 1;
    while (count <= 3)
    {
      argument=one_argument(argument,arg1);
	if (is_number(arg1))
        { 
	  if (strlen(arg1) <= 2)
	  {
	    temp_limit = atoi(arg1);
            if ( (temp_limit < 1) || (temp_limit > 35) )
	      temp_limit = 0;  /* prevents garbage from getting through */
	    if (low_limit == 0)
	      low_limit = temp_limit;
	    else if (high_limit == 0)
	      high_limit = temp_limit;
	  }   /* done parsing for levels */
        }
      else  /* parse for immortal or class flags */
      {
	switch (arg1[0])
	{
	    default:
		break;

            case 'w': case 'W':
                if (check_class == -1)
		  check_class = CLASS_WARRIOR;
                break;
 
            case 'c': case 'C':
                if (check_class == -1)
                  check_class = CLASS_CLERIC;
                break;
 
            case 'm': case 'M':
                if (check_class == -1)
                  check_class = CLASS_MAGIC_USER;
                break;
 
            case 't': case 'T':
                if (check_class == -1)
                  check_class = CLASS_THIEF;
                break;
           
            case 'i': case 'I':
                low_limit = 31;
                high_limit = 35;
         	break;
        }
      } 
    count ++;
    }
    if (low_limit == 0) low_limit = 1;
    if (high_limit == 0) high_limit = 35;

    sprintf(output,"Players\n\r-------\n\r");
    for (d = descriptor_list; d; d = d->next)
    {
	if ((!d->connected) && !IS_NPC(d->character) &&
	    (CAN_SEE(ch, d->character) ) &&
            ((low_limit <= GET_LEVEL(d->character)) &&
             (GET_LEVEL(d->character) <= high_limit)) &&
	    ((check_class == -1)  ||
             ((check_class == CLASS_MAGIC_USER) &&
                (GET_CLASS(d->character) == CLASS_MAGIC_USER))  ||
             ((check_class == CLASS_WARRIOR) &&
                (GET_CLASS(d->character) == CLASS_WARRIOR))     ||
             ((check_class == CLASS_THIEF) &&
                (GET_CLASS(d->character) == CLASS_THIEF))       ||
             ((check_class == CLASS_CLERIC) &&
                (GET_CLASS(d->character) == CLASS_CLERIC))
             ))
	{
	  if (GET_LEVEL(d->character)<10)
	    sprintf(tmp,"  ");
	  else
	    sprintf(tmp," ");

	  sprintf(temp,"MOB");
	  
	  if (GET_CLASS(d->character) == CLASS_WARRIOR)
	    sprintf(temp,"WAR");
	  else
	    if (GET_CLASS(d->character) == CLASS_THIEF)
	      sprintf(temp,"THI");
	    else
	      if (GET_CLASS(d->character) == CLASS_CLERIC)
	    sprintf(temp,"CLE");
	      else
	    if (GET_CLASS(d->character) == CLASS_MAGIC_USER)
	      sprintf(temp,"MAG");
	  
	  if (GET_LEVEL(d->character) == 35)
	    sprintf(temp,"GOD");
	  else
	    if (GET_LEVEL(d->character) == 34)
	      sprintf(temp,"SUP");
	    else
	      if (GET_LEVEL(d->character) == 33)
	    sprintf(temp,"DEI");
	      else
	    if (GET_LEVEL(d->character) == 32)
	      sprintf(temp,"IMM");
	    else
	      if (GET_LEVEL(d->character) == 31)
		sprintf(temp,"HER");

	  if (!strcmp(temp,"MOB"))
	    continue;
	  
	  sprintf(buf, "[%s%d %s ] ",tmp,GET_LEVEL(d->character),temp);
	  strcat(output,buf);
	  temp[0] = '\0';
	  tmp[0]  = '\0';
	  if(IS_SET(d->character->specials.affected_by, AFF_KILLER))
	    sprintf(temp, "(KILLER) ");
	  if(IS_SET(d->character->specials.affected_by, AFF_THIEF))
	    sprintf(tmp, "(THIEF)");
	  if(d->original) /* If switched */{
	    sprintf(buf, "%s %s %s %s\n\r", 
		GET_NAME(d->original),
		d->original->player.title,
		temp,
		tmp);
	  } else {
	    sprintf(buf, "%s %s %s %s\n\r", 
		GET_NAME(d->character),
		d->character->player.title,
		temp,
		tmp);
	  }

          strcat(output,buf);
          found++;
	}
       }
  sprintf(buf,"\n\rPlayers found: %d\n\r",found);
  strcat(output,buf);
  page_string(ch->desc,output,1);
}

void do_sockets(struct char_data *ch, char *argument, int cmd)
{
    char buf[SHORT_STRING_LENGTH];
    char name[MAX_INPUT_LENGTH];
    int num_can_see=0;

    struct descriptor_data *d;

    if ( IS_NPC(ch) )
    {
	send_to_char( "Monsters don't care who's logged in.\n\r", ch );
	return;
    }

    one_argument(argument,name);

    send_to_char( "Socket Stats:\n\r------------\n\r", ch );


    for (d = descriptor_list; d; d = d->next)
    {

	if (d->character == NULL)
	  continue;
	
	if (d->character->player.name == NULL)
	    continue;

	if ( !CAN_SEE(ch, d->character) )
	    continue;

	if ( *name && strcmp( name, d->character->player.name ) )
	    continue;

	num_can_see++;
	sprintf( buf, "%3d : %-30s / %-16s --",
	    d->descriptor, d->host,
	    d->original ? d->original->player.name : d->character->player.name
	    );

	send_to_char(buf, ch);
	switch(d->connected)
	{
	case CON_PLAYING:
	    send_to_char( "CON_PLAYING\n\r", ch ); break;
	case CON_GET_NAME:
	    send_to_char( "CON_GET_NAME\n\r", ch ); break;
	case CON_GET_OLD_PASSWORD:
	    send_to_char( "CON_GET_OLD_PASSWORD\n\r", ch ); break;
	case CON_CONFIRM_NEW_NAME:
	    send_to_char( "CON_CONFIRM_NEW_NAME\n\r", ch ); break;
	case CON_GET_NEW_PASSWORD:
	    send_to_char( "CON_GET_NEW_PASSWORD\n\r", ch ); break;
	case CON_CONFIRM_NEW_PASSWORD:
	    send_to_char( "CON_CONFIRM_NEW_PASSWORD\n\r", ch ); break;
	case CON_GET_NEW_SEX:
	    send_to_char( "CON_GET_NEW_SEX\n\r", ch ); break;
	case CON_GET_NEW_CLASS:
	    send_to_char( "CON_GET_NEW_CLASS\n\r", ch ); break;
	case CON_READ_MOTD:
	    send_to_char( "CON_READ_MOTD\n\r", ch ); break;
        case CON_READ_IMM_MOTD:
            send_to_char( "CON_READ_IMM_MOTD\n\r", ch ); break;
	case CON_SELECT_MENU:
	    send_to_char( "CON_SELECT_MENU\n\r", ch ); break;
	case CON_RESET_PASSWORD:
	    send_to_char( "CON_RESET_PASSWORD\n\r", ch ); break;
	case CON_CONFIRM_RESET_PASSWORD:
	    send_to_char( "CON_CONFIRM_RESET_PASSWORD\n\r", ch ); break;
	case CON_EXDSCR:
	    send_to_char( "CON_EXDSCR\n\r", ch ); break;
	default:
	    send_to_char( "***UNKNOWN***\n\r", ch ); break;
    }


  }
    sprintf(buf,"\n\rThere are %d visible users.\n\r", num_can_see);
    send_to_char(buf,ch);
}




void do_inventory(struct char_data *ch, char *argument, int cmd)
{
    send_to_char("You are carrying:\n\r", ch);
    list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


void do_equipment(struct char_data *ch, char *argument, int cmd)
{
    int j;
    bool found;

    send_to_char("You are using:\n\r", ch);
    found = FALSE;
    for (j=0; j< MAX_WEAR; j++) {
	if (ch->equipment[j]) {
	    if (CAN_SEE_OBJ(ch,ch->equipment[j])) {
		send_to_char(where[j],ch);
		show_obj_to_char(ch->equipment[j],ch,1);
		found = TRUE;
	    } else {
		send_to_char(where[j],ch);
		send_to_char("Something.\n\r",ch);
		found = TRUE;
	    }
	}
    }
    if(!found) {
	send_to_char(" Nothing.\n\r", ch);
    }
}


void do_credits(struct char_data *ch, char *argument, int cmd)
{
    page_string(ch->desc, credits, 0);
}


void do_story(struct char_data *ch, char *argument, int cmd)
{
    page_string(ch->desc, story, 0);
}

void do_news(struct char_data *ch, char *argument, int cmd) {

    page_string(ch->desc, news, 0);
}

void do_motd(struct char_data *ch, char *argument, int cmd) {
 
    page_string(ch->desc, motd, 0);
    send_to_char("\n\r",ch);
}

void do_imotd(struct char_data *ch, char *argument, int cmd) {
 
    page_string(ch->desc, imotd, 0);
    send_to_char("\n\r",ch);
}


void do_info(struct char_data *ch, char *argument, int cmd) {

    page_string(ch->desc, info, 0);
}


void do_wizlist(struct char_data *ch, char *argument, int cmd) {

    page_string(ch->desc, wizlist, 0);
}



void do_where(struct char_data *ch, char *argument, int cmd)
{
    char name[MAX_INPUT_LENGTH], buf[SHORT_STRING_LENGTH], buf2[256];
    register struct char_data *i;
    register struct obj_data *k;
    struct descriptor_data *d;
    int zonenumber;

    one_argument(argument, name);

    if (!*name) {
	if (GET_LEVEL(ch) < 32)
	{
	  zonenumber = ((world[ch->in_room].number)/100);
	  strcpy(buf,
		"Players in your vicinity:\n\r-------------------------\n\r");
	  send_to_char(buf, ch);
	  for (d = descriptor_list; d; d = d->next) {
	    if (d->character && (d->connected == CON_PLAYING) &&
	    (d->character->in_room != NOWHERE) &&
	    CAN_SEE(ch, d->character)) {
	      if (((world[d->character->in_room].number)/100) == zonenumber) {
	    sprintf(buf, "%-20s - %s\n\r",d->character->player.name,
		world[d->character->in_room].name);
	    send_to_char(buf, ch);
	      }
	    }
	  }
	  return;
	}
	else
	{
	    strcpy(buf, "Players:\n\r--------\n\r");
	    send_to_char(buf, ch);
	    for (d = descriptor_list; d; d = d->next) {
		if (d->character && (d->connected == CON_PLAYING) &&
		    (d->character->in_room != NOWHERE) && 
		     CAN_SEE(ch, d->character)) {
		    if (d->original)   /* If switched */
			sprintf(buf, "%-20s - %s [%d] In body of %s\n\r",
			  d->original->player.name,
			  world[d->character->in_room].name,
			  world[d->character->in_room].number,
			  fname(d->character->player.name));
		    else
			sprintf(buf, "%-20s - %s [%d]\n\r",
			  d->character->player.name,
			  world[d->character->in_room].name,
			  world[d->character->in_room].number);
			 
		    send_to_char(buf, ch);
		}
	    }
	    return;
	}
    }

    *buf = '\0';

    for (i = character_list; i; i = i->next)
	if (isname(name, i->player.name) && CAN_SEE(ch, i) )
	{
	    if ((i->in_room != NOWHERE) && ((GET_LEVEL(ch)>31) ||
		(world[i->in_room].zone == world[ch->in_room].zone))) {

		if (IS_NPC(i))
		    sprintf(buf, "%-30s- %s ", i->player.short_descr,
			world[i->in_room].name);
		else
		    sprintf(buf, "%-30s- %s ", i->player.name,
			world[i->in_room].name);

		if (GET_LEVEL(ch) > 31)
		    sprintf(buf2,"[%d]\n\r", world[i->in_room].number);
		else
		    strcpy(buf2, "\n\r");

		strcat(buf, buf2);
		send_to_char(buf, ch);

		if (GET_LEVEL(ch) < 32)
		    break;
	    }
	}

    if (GET_LEVEL(ch) > 31) {
	for (k = object_list; k; k = k->next)
	    if (isname(name, k->name) && CAN_SEE_OBJ(ch, k) && 
		(k->in_room != NOWHERE)) {
		    sprintf(buf, "%-30s- %s [%d]\n\r",
			k->short_description,
			world[k->in_room].name,
			world[k->in_room].number);
			send_to_char(buf, ch);
		}
    }

    if (!*buf)
	send_to_char("Couldn't find any such thing.\n\r", ch);
}




void do_levels(struct char_data *ch, char *argument, int cmd)
{
    int i;
    char buf[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int class;

    if (IS_NPC(ch))
    {
	send_to_char("You ain't nothin' but a hound-dog.\n\r", ch);
	return;
    }
    class = GET_CLASS(ch);

    one_argument(argument,arg);

    if (*arg)
    {
	     if ( !str_cmp( arg, "magic"   ) ) class = 1;
	else if ( !str_cmp( arg, "cleric"  ) ) class = 2;
	else if ( !str_cmp( arg, "thief"   ) ) class = 3;
	else if ( !str_cmp( arg, "fighter" ) ) class = 4;
    }

    for ( i = 1; i <= 31; i++ )
    {
	sprintf( buf, "[%2d] %9d %s\n\r",
	    i,
	    exp_table[i],
	    title_table[class-1][i][GET_SEX(ch)==SEX_FEMALE?1:0]
	    );
	send_to_char( buf, ch );
    }
}



void do_consider(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[256];
    int diff;

    one_argument(argument, name);

    if (!(victim = get_char_room_vis(ch, name))) {
	send_to_char("Consider killing who?\n\r", ch);
	return;
    }

    if (victim == ch) {
	send_to_char("Easy! Very easy indeed!\n\r", ch);
	return;
    }

    if (!IS_NPC(victim)) {
	send_to_char("Would you like to borrow a cross and a shovel?\n\r", ch);
	return;
    }

    diff = (GET_LEVEL(victim)-GET_LEVEL(ch));

    if (diff <= -10)
      send_to_char("Now where did that chicken go?\n\r", ch);
    else if (diff <= -5)
      send_to_char("You could do it with a needle!\n\r", ch);
    else if (diff <= -2)
      send_to_char("Easy.\n\r", ch);
    else if (diff <= -1)
      send_to_char("Fairly easy.\n\r", ch);
    else if (diff == 0)
      send_to_char("The perfect match!\n\r", ch);
    else if (diff <= 1)
      send_to_char("You would need some luck!\n\r", ch);
    else if (diff <= 2)
      send_to_char("You would need a lot of luck!\n\r", ch);
    else if (diff <= 3)
      send_to_char("You would need a lot of luck and great equipment!\n\r",
      ch);
    else if (diff <= 5)
      send_to_char("Do you feel lucky, punk?\n\r", ch);
    else if (diff <= 10)
      send_to_char("Are you mad!?\n\r", ch);
    else if (diff <= 15)
      send_to_char("You ARE mad!\n\r", ch);
    else if (diff <= 20)
      send_to_char(
      "Why don't you just lie down and pretend you are dead instead?\n\r", ch);
    else if (diff <= 25)
      send_to_char("Death will thank you for your gift.\n\r", ch);
    else if (diff <= 30)
      send_to_char("What do you want your epitaph to say?\n\r", ch);
    else if (diff <= 35)
      send_to_char("What ever kills you WILL NOT make you stronger!\n\r", ch);
    else
      send_to_char("Here lies one dead and very dumb MERC player.\n\r", ch );
}
