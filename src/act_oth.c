/***************************************************************************
 *  file: act_oth.c , Implementation of commands.          Part of DIKUMUD *
 *  Usage : Other commands.                                                *
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
#include <string.h>
#include <ctype.h>

#include "structs.h"
#include "mob.h"
#include "obj.h"
#include "utils.h"
#include "interp.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */

extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];


/* extern procedures */

void hit(struct char_data *ch, struct char_data *victim, int type);
void do_shout(struct char_data *ch, char *argument, int cmd);
void do_gossip(struct char_data *ch, char *argument, int cmd);


/* nosummon toggle for mobs and players */

void do_nosummon(struct char_data *ch, char *argument, int cmd)
{
  if (IS_NPC(ch))
  {
    if (IS_SET(ch->specials.act,ACT_NOSUMMON))
    {
	send_to_char("You may now be summoned.\n\r",ch);
	REMOVE_BIT(ch->specials.act,ACT_NOSUMMON);
    }

    else
    {
	send_to_char("You may no longer be summoned.\n\r",ch);
	SET_BIT(ch->specials.act,ACT_NOSUMMON);
    }
  }

  else
  {
    if (IS_SET(ch->specials.act,PLR_NOSUMMON))
    {
        send_to_char("You may now be summoned.\n\r",ch);
        REMOVE_BIT(ch->specials.act,PLR_NOSUMMON);
    }
 
    else
    {
        send_to_char("You may no longer be summoned.\n\r",ch);
        SET_BIT(ch->specials.act,PLR_NOSUMMON);
    }
  }
}

/* vis command for those sneaky thieves */

void do_visible(struct char_data *ch, char *argument, int cmd)
{
  if (IS_AFFECTED(ch,AFF_SNEAK) || affected_by_spell(ch,SKILL_SNEAK))
  {
    send_to_char("You no longer feel so sneaky.\n\r",ch);
    affect_from_char(ch,SKILL_SNEAK);
    REMOVE_BIT(ch->specials.affected_by,AFF_SNEAK);
  }

  if (IS_AFFECTED(ch,AFF_HIDE))
  {
    send_to_char("You are no longer hidden.\n\r",ch);
    REMOVE_BIT(ch->specials.affected_by,AFF_HIDE);
  }

  if (ch->specials.wizInvis)
    do_wizinvis(ch,"",0);

  if (affected_by_spell(ch,SPELL_INVISIBLE))
  {
    send_to_char("You are now visible.\n\r",ch);
    act( "$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM );
    affect_from_char(ch, SPELL_INVISIBLE);
   }
}

/* various AUTO commands -- who needs clients? */


void do_autolist(struct char_data *ch,  char *argument, int cm)
{

    if (IS_NPC(ch))
      return;

    /* lists all AUTO flags the character has set or unset */
    send_to_char("   action     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);

    send_to_char("autoassist     ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOASSIST))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    send_to_char("autoexit       ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOEXIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autogold       ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOGOLD))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autoloot       ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOLOOT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autosac        ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOSAC))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autosplit      ",ch);
    if (IS_SET(ch->specials.act,PLR_AUTOSPLIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("wimpy          ",ch);
    if (IS_SET(ch->specials.act,PLR_WIMPY))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
 
    send_to_char("safe corpse    ",ch);
    if (!IS_SET(ch->specials.act,PLR_CANLOOT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);
}



void do_autosplit(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOSPLIT))
    {
      send_to_char("Autosplitting removed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOSPLIT);
    }
    else
    {
      send_to_char("Autosplitting set.\n\r",ch); 
      SET_BIT(ch->specials.act,PLR_AUTOSPLIT);
    }
}

void do_autoassist(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOASSIST))
    {
      send_to_char("Autoassist removed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOASSIST);
    }
    else
    {
      send_to_char("You will now assist when needed.\n\r",ch);
      SET_BIT(ch->specials.act,PLR_AUTOASSIST);
    }
}

void do_autoloot(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOLOOT))
    {
      send_to_char("Autolooting removed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOLOOT);
    }
    else
    {
      send_to_char("Autolooting set.\n\r",ch);
      SET_BIT(ch->specials.act,PLR_AUTOLOOT);
    }
}

void do_autogold(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOGOLD))
    {
      send_to_char("Autogold removed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOGOLD);
    }
    else
    {
      send_to_char("Autogold set.\n\r",ch);
      SET_BIT(ch->specials.act,PLR_AUTOGOLD);
    }
}

void do_autosac(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOSAC))
    {
      send_to_char("Autosacrificing removed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOSAC);
    }
    else
    {
      send_to_char("Autosacrificing set.\n\r",ch);
      SET_BIT(ch->specials.act,PLR_AUTOSAC);
    }
}

void do_autoexit(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
      return;

    if (IS_SET(ch->specials.act,PLR_AUTOEXIT))
    {
      send_to_char("Exits will no longer be displayed.\n\r",ch);
      REMOVE_BIT(ch->specials.act,PLR_AUTOEXIT);
    }
    else
    {
      send_to_char("Exits will now be displayed.\n\r",ch);
      SET_BIT(ch->specials.act,PLR_AUTOEXIT);
    }
}

void do_qui(struct char_data *ch, char *argument, int cmd)
{
    send_to_char("You have to write quit - no less, to quit!\n\r",ch);
    return;
}

void do_quit(struct char_data *ch, char *argument, int cmd)
{
    int iWear;
    int loss;
    char buf[100];

    if ( IS_NPC(ch) )
	return;

    if (( GET_POS(ch) == POSITION_FIGHTING ) && !IS_SET(ch->specials.affected_by, AFF_KILLER))
    {
	send_to_char( "No way! You are fighting.\n\r", ch );
	return;
    }

    if ( GET_POS(ch) < POSITION_STUNNED )
    {
	send_to_char( "You're not DEAD yet.\n\r", ch );
	return;
    }
    if ( world[ch->in_room].zone != world[real_room(3001)].zone )
       {
        loss = MIN(GET_EXP(ch)/2, 10*GET_LEVEL(ch)*GET_LEVEL(ch) );
        gain_exp(ch, 0 - loss);	
        sprintf( buf, 
        "You lost %d experience for quitting outside of town .\n\r",
            loss );
        send_to_char( buf, ch );
       } /* RT */

    act( "Goodbye, friend.  Come back soon!", FALSE, ch, 0, 0, TO_CHAR );
    act( "$n has left the game.", TRUE, ch, 0, 0, TO_ROOM );

    /* RT Leave gear behind if level 1 */
    
    if (GET_LEVEL(ch) == 1)
    {
      extract_char( ch, TRUE );
      if ( ch->desc)
         close_socket( ch->desc );
      return;
    }

    save_char_obj( ch );
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
    {
	if ( ch->equipment[iWear] )
	    obj_to_char( unequip_char( ch, iWear ), ch );
    }
    while ( ch->carrying )
	extract_obj( ch->carrying );
    extract_char( ch, TRUE );

    /*
     * Can't jam back to menu here because all EQ is gone.
     */
    if ( ch->desc )
	close_socket( ch->desc );
}



void do_save(struct char_data *ch, char *argument, int cmd)
{
    char buf[100];

    if (IS_NPC(ch) || !ch->desc)
	return;

    if ( GET_LEVEL(ch) < 2 )
    {
	send_to_char( "You must be at least second level to save.\n\r", ch );
	return;
    }

    sprintf(buf, "Saving %s.\n\r", GET_NAME(ch));
    send_to_char(buf, ch);
    /* save_char_obj( ch ); */
    ch->specials.will_save = TRUE;  /* it uses the autosaver now */
}



void do_not_here(struct char_data *ch, char *argument, int cmd)
{
    send_to_char("Sorry, but you cannot do that here!\n\r",ch);
}



void do_sneak(struct char_data *ch, char *argument, int cmd)
{
    struct affected_type af;
    byte percent;

    send_to_char("Ok, you'll try to move silently for a while.\n\r", ch);
    if (IS_AFFECTED(ch, AFF_SNEAK))
	affect_from_char(ch, SKILL_SNEAK);

    percent=number(1,101); /* 101% is a complete failure */

    if (percent > ch->skills[SKILL_SNEAK].learned +
	dex_app_skill[GET_DEX(ch)].sneak)
    {
	check_improve(ch,SKILL_SNEAK,1,FALSE);
	return;
    }

    af.type = SKILL_SNEAK;
    af.duration = GET_LEVEL(ch);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_SNEAK;
    affect_to_char(ch, &af);
    check_improve(ch,SKILL_SNEAK,1,TRUE);
}



void do_hide(struct char_data *ch, char *argument, int cmd)
{
    byte percent;

    send_to_char("You attempt to hide yourself.\n\r", ch);

    if (IS_AFFECTED(ch, AFF_HIDE))
	REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

    percent=number(1,101); /* 101% is a complete failure */

    if (percent > ch->skills[SKILL_HIDE].learned +
	dex_app_skill[GET_DEX(ch)].hide)
    {
	check_improve(ch,SKILL_HIDE,1,FALSE);
	return;
    }

    SET_BIT(ch->specials.affected_by, AFF_HIDE);
}


void do_steal(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    struct obj_data *obj;
    char victim_name[240];
    char obj_name[240];
    char buf[240];
    int percent;
    int gold, eq_pos;
    bool ohoh = FALSE;

    argument = one_argument(argument, obj_name);
    one_argument(argument, victim_name);

    if (!(victim = get_char_room_vis(ch, victim_name))) {
	send_to_char("Steal what from who?\n\r", ch);
	return;
    } else if (victim == ch) {
	send_to_char("Come on now, that's rather stupid!\n\r", ch);
	return;
    }

    if (!IS_NPC(victim)) {
      send_to_char(
	"Due to misuse of steal, you can't steal from other players\n\r", ch);
      return;
    }

	if (IS_SET(world[ch->in_room].room_flags,SAFE)){
      send_to_char("No stealing permitted in safe areas!\n\r",ch);
      return;
	}
    
    WAIT_STATE(ch, 10); /* It takes TIME to steal */

    /* 101% is a complete failure */
    percent=number(1,101) - dex_app_skill[GET_DEX(ch)].p_pocket;

   percent += AWAKE(victim) ? 10 : -50;

    if (GET_POS(victim) < POSITION_SLEEPING)
	percent = -1; /* ALWAYS SUCCESS */

    if (GET_LEVEL(victim)>31) /* NO NO With Imp's and Shopkeepers! */
	percent = 101; /* Failure */

    if (str_cmp(obj_name, "coins") && str_cmp(obj_name,"gold")) {

      if (!(obj = get_obj_in_list_vis(victim, obj_name, victim->carrying))) {
	
	for (eq_pos = 0; (eq_pos < MAX_WEAR); eq_pos++)
	  if (victim->equipment[eq_pos] &&
	  (isname(obj_name, victim->equipment[eq_pos]->name)) &&
	  CAN_SEE_OBJ(ch,victim->equipment[eq_pos])) {
	obj = victim->equipment[eq_pos];
	break;
	  }

	if (!obj) {
	  act("$E has not got that item.",FALSE,ch,0,victim,TO_CHAR);
	  return;
	} else { /* It is equipment */
	  if ((GET_POS(victim) > POSITION_STUNNED)) {
	send_to_char("Steal the equipment now? Impossible!\n\r", ch);
	return;
	  } else {
	act("You unequip $p and steal it.",FALSE, ch, obj ,0, TO_CHAR);
	act("$n steals $p from $N.",FALSE,ch,obj,victim,TO_NOTVICT);
	obj_to_char(unequip_char(victim, eq_pos), ch);
	check_improve(ch,SKILL_STEAL,1,TRUE);
	  }
	}
      } else {  /* obj found in inventory */
	
	percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */
	
	if (percent > ch->skills[SKILL_STEAL].learned) {
	  ohoh = TRUE;
	  act("Oops..", FALSE, ch,0,0,TO_CHAR);
	  act("$n tried to steal something from you!",
		FALSE,ch,0,victim,TO_VICT);
	  act("$n tries to steal something from $N.",
		TRUE, ch, 0, victim, TO_NOTVICT);
	  check_improve(ch,SKILL_STEAL,1,FALSE);
	} else { /* Steal the item */
	  if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	  obj_from_char(obj);
	  obj_to_char(obj, ch);
	  send_to_char("Got it!\n\r", ch);
	  check_improve(ch,SKILL_STEAL,1,TRUE);
	  if ((GET_LEVEL(ch)<32) && (!IS_NPC(victim)))
	    {
	      if (!IS_SET(ch->specials.affected_by, AFF_THIEF))
	    {
	      SET_BIT(ch->specials.affected_by, AFF_THIEF);
	      sprintf(log_buf,"%s stole from %s.\n\r",
		GET_NAME(ch),GET_NAME(victim));
	    log(log_buf);
	    }
	    }
	}
	  } else
	send_to_char("You cannot carry that much.\n\r", ch);
	}
      }
    } else { /* Steal some coins */
      if (percent > ch->skills[SKILL_STEAL].learned) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch,0,0,TO_CHAR);
	check_improve(ch,SKILL_STEAL,1,FALSE);
	act("You discover that $n has $s hands in your wallet.",
	    FALSE,ch,0,victim,TO_VICT);
	act("$n tries to steal gold from $N.",
	    TRUE, ch, 0, victim, TO_NOTVICT);
      } else {
	/* Steal some gold coins */
	gold = (int) ((GET_GOLD(victim)*number(1,10))/100);
	gold = MIN(1782, gold);
	if (gold > 0) {
	  GET_GOLD(ch) += gold;
	  GET_GOLD(victim) -= gold;
	  sprintf(buf, "Bingo! You got %d gold coins.\n\r", gold);
	  send_to_char(buf, ch);
	  check_improve(ch,SKILL_STEAL,1,TRUE);
	  if ((GET_LEVEL(ch)<32) && (!IS_NPC(victim)))
	{
	  if (!IS_SET(ch->specials.affected_by, AFF_THIEF))
	    {
	      SET_BIT(ch->specials.affected_by, AFF_THIEF);
	      sprintf(log_buf,"%s stole from %s.\n\r",
		GET_NAME(ch),GET_NAME(victim));
	      log(log_buf);
	    }
	}
	} else {
	  send_to_char("You couldn't get any gold...\n\r", ch);
	}
      }
    }

    if (ohoh && IS_NPC(victim) && AWAKE(victim) && GET_LEVEL(ch)<32){
      if (IS_SET(victim->specials.act, ACT_NICE_THIEF)) {
	sprintf(buf, "%s is a bloody thief.", GET_NAME(ch));
	do_shout(victim, buf, 0);
	send_to_char("Don't you ever do that again!\n\r", ch);
      } else {
	hit(victim, ch, TYPE_UNDEFINED);
      }
    } else {
      if (ohoh && !IS_NPC(victim) && AWAKE(victim)
	  && !IS_SET(ch->specials.affected_by, AFF_THIEF)
	  && GET_LEVEL(ch)<32) {
	SET_BIT(ch->specials.affected_by, AFF_THIEF);
	send_to_char("** You are branded a thief! **\n\r",ch);
	sprintf(buf, "%s is a bloody thief.", GET_NAME(ch)); 
	do_shout(victim, buf, 0);
	log(buf);
	send_to_char(buf,victim);
      }
    }
}



void do_practice(struct char_data *ch, char *arg, int cmd)
{
  /* Call "guild" with a null string for an argument.
     This displays the character's skills. */

  if (arg[0] != '\0'){
    send_to_char("You can only practise in a guild.\n\r", ch);
  } else {
    (void) guild (ch, cmd, "");
  }
}


void do_idea(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[SHORT_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	send_to_char("Monsters can't have ideas - Go away.\n\r", ch);
	return;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument)
    {
	send_to_char("That doesn't sound like a good idea to me.  Sorry.\n\r",
	    ch);
	return;
    }

    if (!(fl = fopen(IDEA_FILE, "a")))
    {
	perror ("do_idea");
	send_to_char("Could not open the idea-file.\n\r", ch);
	return;
    }

    sprintf(str, "**%s[%d]: %s\n",
	GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    fclose(fl);
    send_to_char("Ok.  Thanks.\n\r", ch);
}







void do_typo(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[SHORT_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	send_to_char("Monsters can't spell - leave me alone.\n\r", ch);
	return;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument)
    {
	send_to_char("I beg your pardon?\n\r",
	    ch);
	return;
    }

    if (!(fl = fopen(TYPO_FILE, "a")))
    {
	perror ("do_typo");
	send_to_char("Could not open the typo-file.\n\r", ch);
	return;
    }

    sprintf(str, "**%s[%d]: %s\n",
	GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    fclose(fl);
    send_to_char("Ok.  Thanks.\n\r", ch);
}





void do_bug(struct char_data *ch, char *argument, int cmd)
{
    FILE *fl;
    char str[SHORT_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	send_to_char("You are a monster! Bug off!\n\r", ch);
	return;
    }

    /* skip whites */
    for (; isspace(*argument); argument++);

    if (!*argument)
    {
	send_to_char("Pardon?\n\r",
	    ch);
	return;
    }

    if (!(fl = fopen(BUG_FILE, "a")))
    {
	perror ("do_bug");
	send_to_char("Could not open the bug-file.\n\r", ch);
	return;
    }

    sprintf(str, "**%s[%d]: %s\n",
	GET_NAME(ch), world[ch->in_room].number, argument);
    fputs(str, fl);
    fclose(fl);
    send_to_char("Ok.\n\r", ch);
}



void do_brief(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->specials.act, PLR_BRIEF))
    {
	send_to_char("Brief mode off.\n\r", ch);
	REMOVE_BIT(ch->specials.act, PLR_BRIEF);
    }
    else
    {
	send_to_char("Brief mode on.\n\r", ch);
	SET_BIT(ch->specials.act, PLR_BRIEF);
    }
}


void do_compact(struct char_data *ch, char *argument, int cmd)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->specials.act, PLR_COMPACT))
    {
	send_to_char( "Compact mode off.\n\r", ch);
	REMOVE_BIT(ch->specials.act, PLR_COMPACT);
    }
    else
    {
	send_to_char( "Compact mode on.\n\r", ch);
	SET_BIT(ch->specials.act, PLR_COMPACT);
    }
}



void do_group(struct char_data *ch, char *argument, int cmd)
{
    char name[256];
    char buf[256];
    struct char_data *victim, *k;
    struct follow_type *f;
    bool found;

    one_argument(argument, name);

    if (!*name) {
	if (!IS_AFFECTED(ch, AFF_GROUP)) {
	    send_to_char("But you are a member of no group?!\n\r", ch);
	} else {
	    send_to_char("Your group consists of:\n\r", ch);
	    if (ch->master)
		k = ch->master;
	    else
		k = ch;

	    sprintf(buf,
		"[Lv %2d| %4d/%4dhp %4d/%4dm %4d/%4dmv]\t\t$N (Leader)",
		GET_LEVEL(k),
		GET_HIT(k),
		GET_MAX_HIT(k),
		GET_MANA(k),
		GET_MAX_MANA(k),
		GET_MOVE(k),
		GET_MAX_MOVE(k));
	    act(buf,FALSE,ch, 0, k, TO_CHAR);

	    for(f=k->followers; f; f=f->next) {
	      if (IS_AFFECTED(f->follower, AFF_GROUP)){
		sprintf(buf,"[Lv %2d| %4d/%4dhp %4d/%4dm %4d/%4dmv]\t\t$N",
		    GET_LEVEL(f->follower),
		    GET_HIT(f->follower),
		    GET_MAX_HIT(f->follower),
		    GET_MANA(f->follower),
		    GET_MAX_MANA(f->follower),
		    GET_MOVE(f->follower),
		    GET_MAX_MOVE(f->follower));
		act(buf,FALSE,ch, 0, f->follower, TO_CHAR);
	      }
	    }
	      }
	
	return;
	  }

    if (!(victim = get_char_room_vis(ch, name))) {
	send_to_char("No one here by that name.\n\r", ch);
    } else {

	if (ch->master) {
	    act(
	    "You can not enroll group members without being head of a group.",
	       FALSE, ch, 0, 0, TO_CHAR);
	    return;
	}

	found = FALSE;

	if (victim == ch)
	    found = TRUE;
	else {
	    for(f=ch->followers; f; f=f->next) {
		if (f->follower == victim) {
		    found = TRUE;
		    break;
		}
	    }
	}
	
	if (found) {
	  if (abs(GET_LEVEL(ch)-GET_LEVEL(victim)) < 9){
	    if (IS_AFFECTED(victim, AFF_GROUP)) {
		act("$n has been kicked out of the group!",
		    FALSE, victim, 0, ch, TO_ROOM);
		act("You are no longer a member of the group!",
		    FALSE, victim, 0, 0, TO_CHAR);
		REMOVE_BIT(victim->specials.affected_by, AFF_GROUP);
                if (victim == ch)
                { /* dissolve group */
                   for (f=ch->followers; f; f=f->next)
                      {
			if (IS_AFFECTED(f->follower, AFF_GROUP))
                        {
		           act("$n has been kicked out of the group!",
                           FALSE, f->follower, 0, ch, TO_ROOM);
                	   act("You are no longer a member of the group!",
                           FALSE, f->follower, 0, 0, TO_CHAR);
                           REMOVE_BIT(f->follower->specials.affected_by, AFF_GROUP);
                         }
                       }
                 }
	    } else {   
		act("$n is now a group member.",
		    FALSE, victim, 0, 0, TO_ROOM);
		act("You are now a group member.",
		    FALSE, victim, 0, 0, TO_CHAR);
		SET_BIT(victim->specials.affected_by, AFF_GROUP);
                /* add leader if he is not yet a member */
                if (!IS_AFFECTED(ch,AFF_GROUP))
                   {
                act("$n is now a group member.",
                    FALSE, ch, 0, 0, TO_ROOM);
                act("You are now a group member.",
                    FALSE, ch, 0, 0, TO_CHAR);
                SET_BIT(ch->specials.affected_by, AFF_GROUP);
                }
	    }
	      } else {
	    act("$n is not of the right caliber to join this group.",
	   	 FALSE, victim, 0, 0, TO_ROOM);
	      }
	} else {
	  act("$N must follow you, to enter the group.",
	  FALSE, ch, 0, victim, TO_CHAR);
	}
    }
}


void do_quaff(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
  struct obj_data *temp;
  int i;
    bool equipped;

    equipped = FALSE;

  one_argument(argument,buf);

    if (!(temp = get_obj_in_list_vis(ch,buf,ch->carrying))) {
	temp = ch->equipment[HOLD];
	equipped = TRUE;
      if ((temp==0) || !isname(buf, temp->name)) {
	    act("You do not have that item.",FALSE,ch,0,0,TO_CHAR);
      return;
    }
    }

  if (temp->obj_flags.type_flag!=ITEM_POTION)
  {
    act("You can only quaff potions.",FALSE,ch,0,0,TO_CHAR);
    return;
  }

  act("$n quaffs $p.", TRUE, ch, temp, 0, TO_ROOM);
  act("You quaff $p which dissolves.",FALSE,ch,temp,0,TO_CHAR);

  for (i=1; i<4; i++)
    if (temp->obj_flags.value[i] >= 1)
      ((*spell_info[temp->obj_flags.value[i]].spell_pointer)
	((byte) temp->obj_flags.value[0], ch, "", SPELL_TYPE_POTION, ch, 0));

    if (equipped)
	unequip_char(ch, HOLD);

    extract_obj(temp);
}


void do_recite(struct char_data *ch, char *argument, int cmd)
{
    char buf[100];
    struct obj_data *scroll, *obj;
    struct char_data *victim;
    int i, bits;
    bool equipped;

    equipped = FALSE;
    obj = 0;
    victim = 0;

    argument = one_argument(argument,buf);

    if (!(scroll = get_obj_in_list_vis(ch,buf,ch->carrying))) {
	scroll = ch->equipment[HOLD];
	equipped = TRUE;
      if ((scroll==0) || !isname(buf, scroll->name)) {
	    act("You do not have that item.",FALSE,ch,0,0,TO_CHAR);
	    return;
    }
    }

  if (scroll->obj_flags.type_flag!=ITEM_SCROLL)
  {
    act("Recite is normally used for scroll's.",FALSE,ch,0,0,TO_CHAR);
    return;
  }

    if (*argument) {
      bits = generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM |
	FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &victim, &obj);
	if (bits == 0) {
	    send_to_char(
		"No such thing around to recite the scroll on.\n\r", ch);
	    return;
	}
    } else {
	victim = ch;
    }

    act("$n recites $p.", TRUE, ch, scroll, 0, TO_ROOM);
  act("You recite $p which dissolves.",FALSE,ch,scroll,0,TO_CHAR);

  for (i=1; i<4; i++)
    if (scroll->obj_flags.value[i] >= 1)
      ((*spell_info[scroll->obj_flags.value[i]].spell_pointer)
      ((byte) scroll->obj_flags.value[0],
      ch, "", SPELL_TYPE_SCROLL, victim, obj));

    if (equipped)
	unequip_char(ch, HOLD);

    extract_obj(scroll);
}



void do_use(struct char_data *ch, char *argument, int cmd)
{
  char buf[100];
    struct char_data *tmp_char;
  struct obj_data *tmp_object, *stick;

  int bits;

  argument = one_argument(argument,buf);

  if (ch->equipment[HOLD] == 0 ||
      !isname(buf, ch->equipment[HOLD]->name)) {
    act("You do not hold that item in your hand.",FALSE,ch,0,0,TO_CHAR);
    return;
  }

    stick = ch->equipment[HOLD];

  if (stick->obj_flags.type_flag == ITEM_STAFF)
  {
	act("$n taps $p three times on the ground.",
	    TRUE, ch, stick, 0,TO_ROOM);
	act("You tap $p three times on the ground.",
	    FALSE,ch, stick,0,TO_CHAR);

	if (stick->obj_flags.value[2] > 0) { /* Charges left? */
	    stick->obj_flags.value[2]--;
	    ((*spell_info[stick->obj_flags.value[3]].spell_pointer)
	    ((byte) stick->obj_flags.value[0], ch, "",
	    SPELL_TYPE_STAFF, 0, 0));

	} else {
	    send_to_char("The staff disintegrates in your hands.\n\r", ch);
            act
	    ("The staff disintegrates in $n's hands!",FALSE,ch,0,0,TO_ROOM);
            unequip_char(ch, HOLD);
	    extract_obj(stick);
	}
    } else if (stick->obj_flags.type_flag == ITEM_WAND) {

	bits = generic_find(argument, FIND_CHAR_ROOM | FIND_OBJ_INV
	| FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
	if (bits) {
	    if (bits == FIND_CHAR_ROOM) {
		act("$n points $p at $N.", TRUE, ch, stick, tmp_char, TO_ROOM);
		act("You point $p at $N.",FALSE,ch, stick, tmp_char, TO_CHAR);
	    } else {
	    act("$n point $p at $P.", TRUE, ch, stick, tmp_object, TO_ROOM);
	    act("You point $p at $P.",FALSE,ch, stick, tmp_object, TO_CHAR);
	    }

	    if (stick->obj_flags.value[2] > 0) {
	    /* Is there any charges left? */
		stick->obj_flags.value[2]--;
		((*spell_info[stick->obj_flags.value[3]].spell_pointer)
	      ((byte) stick->obj_flags.value[0], ch, "",
	      SPELL_TYPE_WAND, tmp_char, tmp_object));
	    } else {
		send_to_char("The wand vibrates and explodes!\n\r", ch);
		act("The wand vibrates and explodes!",FALSE,ch,0,0,TO_ROOM);
                unequip_char(ch, HOLD);
                extract_obj(stick);
	    }

	} else {
	    send_to_char("What should the wand be pointed at?\n\r", ch);
	}
    } else {
	send_to_char("Use is normally only for wand's and staff's.\n\r", ch);
  }
}


void do_recall( CHAR_DATA *ch, char *argument, int cmd )
/* RT modified: no recalls at temple, automatic non-combat recall */
{
    int location;
    int percent;
    CHAR_DATA *victim;
  
    percent = number(1, 101);
    if (ch->in_room != real_room(3001) )
      act( "$n prays to $s God for transportation!",
	TRUE, ch, 0, 0, TO_ROOM );

    if (( percent > /* ch->skills[SKILL_RECALL].learned */ 50 ) &&
          ( GET_POS(ch) == POSITION_FIGHTING ))
    {
	send_to_char( "You failed in your recall!\n\r", ch );
	return;
    }
  
    if ( ( location = real_room(3001) ) == -1)
    {
	send_to_char( "You are completely lost.\n\r", ch );
	return;
    }
    if ( ch->in_room == real_room(3001) )
    {
        send_to_char( "You are already there!\n\r", ch);
        return;
    }

    if ( ( victim = ch->specials.fighting ) != NULL )
    {
	/*
	 * Need to stop fighting somehow too.
	 */
	gain_exp( ch,
	    0 - abs( GET_LEVEL(victim)
	    * (GET_MAX_HIT(victim) - GET_HIT(victim) ) ) );
    }

    act( "$n disappears.", TRUE, ch, 0, 0, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM );
    do_look( ch, "", 0 );

    return;
}



/*****************************************************************/
/* New code : by Dionysos.                                       */
/*****************************************************************/

void do_wimpy(struct char_data *ch, char *argument, int cmd)
{
    if (IS_SET(ch->specials.act, PLR_WIMPY))
    {
	send_to_char("You are no longer a wimp....maybe.\n\r", ch);
	REMOVE_BIT(ch->specials.act, PLR_WIMPY);
    }
    else
    {
	send_to_char("You are now an official wimp.\n\r", ch);
	SET_BIT(ch->specials.act, PLR_WIMPY);
    }
}

