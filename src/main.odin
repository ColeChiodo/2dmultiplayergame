package main

import rl "vendor:raylib"
import "core:fmt"
import "core:math"

GRAVITY :: 1800.0
DT :: 1.0 / 60.0
GROUND_HEIGHT :: 450

SCREEN_WIDTH  :: 640
SCREEN_HEIGHT :: 480
STAGE_LEFT    :: 0.0
STAGE_RIGHT   :: SCREEN_WIDTH * 2.5
MAX_DISTANCE  :: SCREEN_WIDTH * 1.5

MAX_PLAYERS :: 2
INPUT_BUFFER_SIZE :: 120
INPUT_HISTORY_SIZE :: 10
STATE_HISTORY_SIZE :: 60
INPUT_BUFFER_WINDOW :: 6
THROW_TECH_WINDOW :: 7
KNOCKDOWN_GROUND_TIME :: 180
KNOCKDOWN_FORCED_GROUND :: 30
WAKEUP_DURATION :: 10
KNOCKDOWN_INVINCIBLE :: 60
ROLL_DURATION :: 8
ROLL_SPEED :: 600.0
ROLL_INVINCIBLE :: 5

MAX_HURTBOXES :: 4
MAX_HITBOXES :: 4
MAX_HITBOX_STRIPS :: 8
MAX_HITBOXES_PER_STRIP :: 4
MAX_COLLISION_BOXES :: 4
MAX_COLLISION_BOXES_PER_STRIP :: 4
MAX_COLLISION_STRIPS :: 8
MAX_HURTBOX_STRIPS :: 8
MAX_ANIMATION_STRIPS :: 8

attack_type :: enum {
	atk_5l,
	atk_5m,
	atk_5h,
	atk_throw,
	atk_2l,
	atk_2m,
	atk_2h,
	atk_count,
}

player_state :: enum {
	ps_idle,
	ps_walk,
	ps_crouch,
	ps_jump,
	ps_attack,
	ps_hitstun,
	ps_blockstun,
	ps_block,
	ps_throwing,
	ps_thrown,
	ps_knockdown,
	ps_wakeup,
	ps_rolling,
}

display_state :: enum {
	ds_idle,
	ds_walk,
	ds_jump,
	ds_attack_startup,
	ds_attack_active,
	ds_attack_recovery,
	ds_hitstun,
	ds_blockstun,
	ds_throwing,
	ds_thrown,
	ds_knockdown,
	ds_knockdown_forced,
	ds_wakeup,
	ds_rolling,
}

input_state :: struct {
	left, right, up, down: bool,
	light, medium, heavy: bool,
}

input_history_entry :: struct {
	state: input_state,
	duration: int,
}

rect :: struct {
	offset_x, offset_y: f32,
	width, height: f32,
}

hitbox :: struct {
	rect: rect,
}

collision_box :: struct {
	rect: rect,
}

animation_strip :: struct {
	start_frame, end_frame: int,
	collision_box_count: int,
	collision_boxes: [MAX_COLLISION_BOXES_PER_STRIP]collision_box,
	hurtbox_count: int,
	hurtboxes: [MAX_HURTBOXES]rect,
}

animation_strips :: struct {
	strip_count: int,
	strips: [MAX_ANIMATION_STRIPS]animation_strip,
}

hitbox_strip :: struct {
	start_frame, end_frame: int,
	hitbox_count: int,
	hitboxes: [MAX_HITBOXES_PER_STRIP]hitbox,
	damage: f32,
	knockback_x, knockback_y: f32,
	block_knockback_x, block_knockback_y: f32,
	hitstun, blockstun: int,
	overhead, low: bool,
	knockdown: bool,
}

hit_result :: struct {
	hit: bool,
	damage: f32,
	hitstun: int,
	blockstun: int,
	knockback_x: f32,
	knockback_y: f32,
	block_knockback_x: f32,
	block_knockback_y: f32,
	overhead: bool,
	low: bool,
	knockdown: bool,
}

attack_data :: struct {
	startup, active, recovery: int,
	strips: [MAX_HITBOX_STRIPS]hitbox_strip,
	strip_count: int,
	animations: animation_strips,
	move_x, move_y: f32,
}

throw_data :: struct {
	startup: int,
	active: int,
	whiff_recovery: int,
	throw_duration: int,
	throw_recovery: int,
	range: f32,
	damage: f32,
	hitstun: int,
	knockback_x, knockback_y: f32,
	knockdown: bool,
	startup_anim: animation_strips,
}

animation_set :: struct {
	idle: animation_strips,
	walk: animation_strips,
	crouch: animation_strips,
	jump: animation_strips,
	blockstun: animation_strips,
	crouch_blockstun: animation_strips,
	hitstun: animation_strips,
	crouch_hitstun: animation_strips,
	throw_anim: animation_strips,
	thrown_anim: animation_strips,
	knockdown: animation_strips,
	wakeup: animation_strips,
	rolling: animation_strips,
}

character_data :: struct {
	name: cstring,
	walk_speed: f32,
	jump_strength: f32,
	width: f32,
	height: f32,
	max_health: f32,
	animations: animation_set,
	attacks: [attack_type.atk_count]attack_data,
	throw_data: throw_data,
}

player :: struct {
	character: ^character_data,
	x, y: f32,
	velocity_x, velocity_y: f32,
	grounded: bool,
	state: player_state,
	state_timer: int,
	facing_right: bool,
	attack_type: attack_type,
	attack_frame: int,
	health: f32,
	hit_connected: [MAX_HITBOX_STRIPS]bool,
	stun_anim_state: int,
	buffered_attack: int,
	buffer_timer: int,
	chord_timer: int,
	chord_intent: int,
	invincible_timer: int,
	wakeup_attack: bool,
	input_lock: int,
}

game_input :: struct {
	players: [MAX_PLAYERS]input_state,
}

game_state :: struct {
	players: [MAX_PLAYERS]player,
	input_buffer: [INPUT_BUFFER_SIZE]game_input,
	current_frame: int,
}

save_state :: struct {
	x, y: f32,
	velocity_x, velocity_y: f32,
	grounded: bool,
	facing_right: bool,
	state: player_state,
	state_timer: int,
	attack_type: int,
	health: f32,
	hit_connected: [MAX_HITBOX_STRIPS]bool,
	stun_anim_state: int,
	buffered_attack: int,
	buffer_timer: int,
	chord_timer: int,
	chord_intent: int,
	invincible_timer: int,
	wakeup_attack: bool,
	input_lock: int,
}

frame_meter_entry :: struct {
	p1: display_state,
	p2: display_state,
}

FRAME_METER_MAX :: 60

character_1 := character_data{
	name = "Character 1",
	walk_speed = 300.0,
	jump_strength = 800.0,
	width = 75.0,
	height = 150.0,
	max_health = 10000.0,
	animations = {
		idle = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		walk = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		crouch = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -75.0, width = 75.0, height = 75.0},
					},
				},
			},
		},
		jump = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		blockstun = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		crouch_blockstun = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -75.0, width = 75.0, height = 75.0},
					},
				},
			},
		},
		hitstun = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		crouch_hitstun = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -75.0, width = 75.0, height = 75.0},
					},
				},
			},
		},
		throw_anim = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 19,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 0,
				},
			},
		},
		thrown_anim = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 19,
					collision_box_count = 0,
					hurtbox_count = 0,
				},
			},
		},
		knockdown = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -37.5, offset_y = -15.0, width = 75.0, height = 15.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -15.0, width = 75.0, height = 15.0},
					},
				},
			},
		},
		wakeup = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
		rolling = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 0,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -37.5, offset_y = -15.0, width = 75.0, height = 15.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -15.0, width = 75.0, height = 15.0},
					},
				},
			},
		},
	},
	attacks = {
		attack_type.atk_5l = attack_data{
			startup = 2, active = 5, recovery = 3,
			move_x = 0.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 2, end_frame = 6,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 37.5, offset_y = -90.0, width = 45.0, height = 30.0}},
					},
					damage = 100,
					knockback_x = 100.0, knockback_y = 0.0,
					block_knockback_x = 100.0, block_knockback_y = 0.0,
					hitstun = 10,
					blockstun = 9,
					low = false,
					overhead = false,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 9,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
						},
					},
				},
			},
		},
		attack_type.atk_5m = attack_data{
			startup = 5, active = 5, recovery = 8,
			move_x = 0.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 5, end_frame = 9,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 67.5, offset_y = -60.0, width = 52.5, height = 37.5}},
					},
					damage = 200,
					knockback_x = 150.0, knockback_y = 0.0,
					block_knockback_x = 150.0, block_knockback_y = 0.0,
					hitstun = 15,
					blockstun = 12,
					low = false,
					overhead = false,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 17,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
						},
					},
				},
			},
		},
		attack_type.atk_5h = attack_data{
			startup = 8, active = 10, recovery = 10,
			move_x = 300.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 8, end_frame = 17,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 75.0, offset_y = -45.0, width = 60.0, height = 45.0}},
					},
					damage = 400,
					knockback_x = 300.0, knockback_y = -300.0,
					block_knockback_x = 200.0, block_knockback_y = 50.0,
					hitstun = 25,
					blockstun = 12,
					low = true,
					overhead = false,
					knockdown = true,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 27,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
						},
					},
				},
			},
		},
		attack_type.atk_2l = attack_data{
			startup = 3, active = 3, recovery = 4,
			move_x = 0.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 3, end_frame = 5,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 37.5, offset_y = -60.0, width = 45.0, height = 25.0}},
					},
					damage = 80,
					knockback_x = 80.0, knockback_y = 0.0,
					block_knockback_x = 60.0, block_knockback_y = 0.0,
					hitstun = 8,
					blockstun = 7,
					low = true,
					overhead = false,
					knockdown = false,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 9,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -75.0, width = 75.0, height = 75.0},
						},
					},
				},
			},
		},
		attack_type.atk_2m = attack_data{
			startup = 6, active = 5, recovery = 10,
			move_x = 0.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 6, end_frame = 10,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 67.5, offset_y = -45.0, width = 52.5, height = 30.0}},
					},
					damage = 180,
					knockback_x = 120.0, knockback_y = 0.0,
					block_knockback_x = 100.0, block_knockback_y = 0.0,
					hitstun = 13,
					blockstun = 10,
					low = true,
					overhead = false,
					knockdown = false,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 20,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -100.0, width = 75.0, height = 100.0},
						},
					},
				},
			},
		},
		attack_type.atk_2h = attack_data{
			startup = 10, active = 8, recovery = 14,
			move_x = 200.0, move_y = 0.0,
			strip_count = 1,
			strips = {
				0 = hitbox_strip{
					start_frame = 10, end_frame = 17,
					hitbox_count = 1,
					hitboxes = {
						0 = hitbox{rect = rect{offset_x = 75.0, offset_y = -30.0, width = 60.0, height = 25.0}},
					},
					damage = 350,
					knockback_x = 250.0, knockback_y = -250.0,
					block_knockback_x = 150.0, block_knockback_y = 50.0,
					hitstun = 22,
					blockstun = 10,
					low = true,
					overhead = false,
					knockdown = true,
				},
			},
			animations = {
				strip_count = 1,
				strips = {
					0 = animation_strip{
						start_frame = 0, end_frame = 31,
						collision_box_count = 1,
						collision_boxes = {
							0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -75.0, width = 44.0, height = 75.0}},
						},
						hurtbox_count = 1,
						hurtboxes = {
							0 = rect{offset_x = -37.5, offset_y = -75.0, width = 75.0, height = 75.0},
						},
					},
				},
			},
		},
	},
	throw_data = {
		startup = 3,
		active = 1,
		whiff_recovery = 12,
		throw_duration = 20,
		throw_recovery = 5,
		range = 75.0,
		damage = 800,
		hitstun = 25,
		knockback_x = 0.0,
		knockback_y = -200.0,
		knockdown = true,
		startup_anim = {
			strip_count = 1,
			strips = {
				0 = animation_strip{
					start_frame = 0, end_frame = 15,
					collision_box_count = 1,
					collision_boxes = {
						0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
					},
					hurtbox_count = 1,
					hurtboxes = {
						0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
					},
				},
			},
		},
	},
}

gs: game_state
sim_fps: f32 = 0.0
sim_frame_time_ms: f32 = 0.0

history_p1: [INPUT_HISTORY_SIZE]input_history_entry
history_p2: [INPUT_HISTORY_SIZE]input_history_entry
history_count_p1: int = 0
history_count_p2: int = 0
show_input_p1: bool = true
show_input_p2: bool = false
show_state_timeline: bool = true

state_history_p1: [STATE_HISTORY_SIZE]display_state
state_history_p2: [STATE_HISTORY_SIZE]display_state

frame_meter: [FRAME_METER_MAX]frame_meter_entry
frame_meter_count: int = 0
meter_p1_attack_type: int = -1
meter_p2_attack_type: int = -1
meter_last_adv_p1: int = 0
meter_last_adv_p2: int = 0
frame_meter_active: bool = false
frame_meter_has_data: bool = false

getup_attack_data := attack_data{
	startup = 5,
	active = 8,
	recovery = 22,
	move_x = 0.0, move_y = 0.0,
	animations = {
		strip_count = 1,
		strips = {
			0 = animation_strip{
				start_frame = 0, end_frame = 34,
				collision_box_count = 1,
				collision_boxes = {
					0 = collision_box{rect = rect{offset_x = -22.0, offset_y = -90.0, width = 44.0, height = 85.0}},
				},
				hurtbox_count = 1,
				hurtboxes = {
					0 = rect{offset_x = -37.5, offset_y = -150.0, width = 75.0, height = 150.0},
				},
			},
		},
	},
	strip_count = 1,
	strips = {
		0 = hitbox_strip{
			start_frame = 5, end_frame = 12,
			hitbox_count = 1,
			hitboxes = {
				0 = hitbox{rect = rect{offset_x = -75.0, offset_y = -90.0, width = 150.0, height = 90.0}},
			},
			damage = 250,
			knockback_x = 400.0, knockback_y = -100.0,
			block_knockback_x = 200.0, block_knockback_y = 0.0,
			hitstun = 20,
			blockstun = 8,
			overhead = false, low = false, knockdown = false,
		},
	},
}

aabb :: proc(ax, ay, aw, ah, bx, by, bw, bh: f32) -> bool {
	return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by
}

world_rect :: proc(p: ^player, r: ^rect) -> (f32, f32) {
	return p.x + (p.facing_right ? r.offset_x : -r.offset_x - r.width),
	       p.y + r.offset_y
}

get_current_animation_strips :: proc(p: ^player) -> ^animation_strips {
	switch p.state {
	case .ps_idle:   return &p.character.animations.idle
	case .ps_walk:   return &p.character.animations.walk
	case .ps_crouch: return &p.character.animations.crouch
	case .ps_jump:   return &p.character.animations.jump
	case .ps_attack:
		if p.wakeup_attack { return &getup_attack_data.animations }
		if p.attack_type == .atk_throw { return &p.character.throw_data.startup_anim }
		return &p.character.attacks[p.attack_type].animations
	case .ps_block:
		if p.stun_anim_state == 1 { return &p.character.animations.crouch }
		return &p.character.animations.idle
	case .ps_hitstun:
		if p.stun_anim_state == 1 { return &p.character.animations.crouch_hitstun }
		return &p.character.animations.hitstun
	case .ps_blockstun:
		if p.stun_anim_state == 1 { return &p.character.animations.crouch_blockstun }
		return &p.character.animations.blockstun
	case .ps_throwing: return &p.character.animations.throw_anim
	case .ps_thrown:   return &p.character.animations.thrown_anim
	case .ps_knockdown: return &p.character.animations.knockdown
	case .ps_wakeup:    return &p.character.animations.wakeup
	case .ps_rolling:   return &p.character.animations.rolling
	}
	return nil
}

get_current_animation_frame :: proc(p: ^player) -> int {
	#partial switch p.state {
	case .ps_attack:
		if p.attack_type == .atk_throw {
			td := &p.character.throw_data
			total := td.startup + td.active + td.whiff_recovery
			return total - p.state_timer
		}
		attack_data := p.wakeup_attack ? &getup_attack_data : &p.character.attacks[p.attack_type]
		total := attack_data.startup + attack_data.active + attack_data.recovery
		return total - p.state_timer
	case .ps_throwing, .ps_thrown:
		td := &p.character.throw_data
		return td.throw_duration - p.state_timer
	case .ps_knockdown, .ps_wakeup:
		return 0
	case:
		return 0
	}
}

get_active_collision_boxes :: proc(p: ^player, out_boxes: []collision_box, max_out: int) -> int {
	anim := get_current_animation_strips(p)
	if anim == nil { return 0 }

	frame := get_current_animation_frame(p)
	count := 0
	for s in 0..<anim.strip_count {
		if count >= max_out { break }
		strip := &anim.strips[s]
		if frame >= strip.start_frame && frame <= strip.end_frame {
			for b in 0..<strip.collision_box_count {
				if count >= max_out { break }
				out_boxes[count] = strip.collision_boxes[b]
				count += 1
			}
		}
	}
	return count
}

get_active_hurtboxes :: proc(p: ^player, out_boxes: []rect, max_out: int) -> int {
	anim := get_current_animation_strips(p)
	if anim == nil { return 0 }

	frame := get_current_animation_frame(p)
	count := 0
	for s in 0..<anim.strip_count {
		if count >= max_out { break }
		strip := &anim.strips[s]
		if frame >= strip.start_frame && frame <= strip.end_frame {
			for h in 0..<strip.hurtbox_count {
				if count >= max_out { break }
				out_boxes[count] = strip.hurtboxes[h]
				count += 1
			}
		}
	}
	return count
}

get_collision_hull :: proc(p: ^player) -> (min_x, min_y, max_x, max_y: f32) {
	boxes: [MAX_COLLISION_BOXES]collision_box
	count := get_active_collision_boxes(p, boxes[:], MAX_COLLISION_BOXES)

	if count == 0 {
		return p.x - p.character.width * 0.5,
		       p.y - p.character.height,
		       p.x + p.character.width * 0.5,
		       p.y
	}

	min_x = math.INF_F32
	min_y = math.INF_F32
	max_x = -math.INF_F32
	max_y = -math.INF_F32

	for i in 0..<count {
		wx, wy := world_rect(p, &boxes[i].rect)
		wr := wx + boxes[i].rect.width
		wb := wy + boxes[i].rect.height
		if wx < min_x { min_x = wx }
		if wy < min_y { min_y = wy }
		if wr > max_x { max_x = wr }
		if wb > max_y { max_y = wb }
	}
	return
}

resolve_player_collision :: proc(p0, p1: ^player) {
	min0x, min0y, max0x, max0y := get_collision_hull(p0)
	min1x, min1y, max1x, max1y := get_collision_hull(p1)

	if min0x < max1x && max0x > min1x && min0y < max1y && max0y > min1y {
		overlap_x := math.min(max0x - min1x, max1x - min0x)
		push := overlap_x * 0.5

		center0 := (min0x + max0x) * 0.5
		center1 := (min1x + max1x) * 0.5
		direction: f32
		if center0 < center1 {
			direction = -1.0
		} else if center0 > center1 {
			direction = 1.0
		} else {
			stage_center := (STAGE_LEFT + STAGE_RIGHT) * 0.5
			if p0.y < p1.y {
			direction = p0.x < f32(stage_center) ? 1.0 : -1.0
		} else {
			direction = p1.x < f32(stage_center) ? -1.0 : 1.0
			}
		}

		p0.x += direction * push
		p1.x -= direction * push
	}
}

save_player_state :: proc(player: ^player, save: ^save_state) {
	save.x = player.x
	save.y = player.y
	save.velocity_x = player.velocity_x
	save.velocity_y = player.velocity_y
	save.grounded = player.grounded
	save.facing_right = player.facing_right
	save.state = player.state
	save.state_timer = player.state_timer
	save.attack_type = int(player.attack_type)
	save.health = player.health
	save.stun_anim_state = player.stun_anim_state
	save.buffered_attack = player.buffered_attack
	save.buffer_timer = player.buffer_timer
	save.chord_timer = player.chord_timer
	save.chord_intent = player.chord_intent
	save.invincible_timer = player.invincible_timer
	save.wakeup_attack = player.wakeup_attack
	save.input_lock = player.input_lock
	for i in 0..<MAX_HITBOX_STRIPS {
		save.hit_connected[i] = player.hit_connected[i]
	}
}

restore_player_state :: proc(player: ^player, save: ^save_state) {
	player.x = save.x
	player.y = save.y
	player.velocity_x = save.velocity_x
	player.velocity_y = save.velocity_y
	player.grounded = save.grounded
	player.facing_right = save.facing_right
	player.state = save.state
	player.state_timer = save.state_timer
	player.attack_type = attack_type(save.attack_type)
	player.health = save.health
	player.stun_anim_state = save.stun_anim_state
	player.buffered_attack = save.buffered_attack
	player.buffer_timer = save.buffer_timer
	player.chord_timer = save.chord_timer
	player.chord_intent = save.chord_intent
	player.invincible_timer = save.invincible_timer
	player.wakeup_attack = save.wakeup_attack
	player.input_lock = save.input_lock
	for i in 0..<MAX_HITBOX_STRIPS {
		player.hit_connected[i] = save.hit_connected[i]
	}
}

start_attack :: proc(player: ^player, atk_type: attack_type) {
	player.attack_type = atk_type
	attack_data := &player.character.attacks[atk_type]
	player.state_timer = attack_data.startup + attack_data.active + attack_data.recovery
	player.attack_frame = 0
	for i in 0..<MAX_HITBOX_STRIPS {
		player.hit_connected[i] = false
	}
	if player.grounded {
		player.velocity_x = 0.0
	}
	player.state = .ps_attack
}

start_throw :: proc(player: ^player) {
	player.attack_type = .atk_throw
	td := &player.character.throw_data
	player.state_timer = td.startup + td.active + td.whiff_recovery
	player.attack_frame = 0
	for i in 0..<MAX_HITBOX_STRIPS {
		player.hit_connected[i] = false
	}
	if player.grounded {
		player.velocity_x = 0.0
	}
	player.state = .ps_attack
}

start_wakeup_attack :: proc(player: ^player) {
	player.state = .ps_attack
	player.attack_type = .atk_5l
	player.wakeup_attack = true
	player.state_timer = getup_attack_data.startup + getup_attack_data.active + getup_attack_data.recovery
	player.attack_frame = 0
	for i in 0..<MAX_HITBOX_STRIPS {
		player.hit_connected[i] = false
	}
	player.velocity_x = 0.0
	player.velocity_y = 0.0
}

init_player :: proc(player: ^player, x, y: f32, character: ^character_data) {
	player.x = x
	player.y = y
	player.velocity_x = 0.0
	player.velocity_y = 0.0
	player.grounded = false
	player.facing_right = true
	player.state = .ps_idle
	player.character = character
	player.state_timer = 0
	player.attack_type = .atk_5l
	player.attack_frame = 0
	player.health = character.max_health
	player.stun_anim_state = 0
	player.buffered_attack = -1
	player.buffer_timer = 0
	player.chord_timer = 0
	player.chord_intent = 0
	player.invincible_timer = 0
	player.wakeup_attack = false
	player.input_lock = 0
	for i in 0..<MAX_HITBOX_STRIPS {
		player.hit_connected[i] = false
	}
}

game_step :: proc(player, other: ^player, input, prev_input: ^input_state) {
	direction_x := 0
	if input.left && !input.right {
		direction_x = -1
	} else if input.right && !input.left {
		direction_x = 1
	}

	if player.grounded && (player.state == .ps_idle || player.state == .ps_walk || player.state == .ps_crouch) {
		dx := other.x - player.x
		if dx > 0 {
			player.facing_right = true
		} else if dx < 0 {
			player.facing_right = false
		}
	}

	if player.buffer_timer > 0 {
		player.buffer_timer -= 1
		if player.buffer_timer == 0 {
			player.buffered_attack = -1
		}
	}

	if player.invincible_timer > 0 {
		player.invincible_timer -= 1
	}

	if player.input_lock > 0 {
		player.input_lock -= 1
	}

	light_pressed := input.light && !prev_input.light
	medium_pressed := input.medium && !prev_input.medium
	heavy_pressed := input.heavy && !prev_input.heavy
	throw_pressed := (input.medium && input.heavy) && !(prev_input.medium && prev_input.heavy)

	any_pressed := light_pressed || medium_pressed || heavy_pressed || throw_pressed
	can_act := player.state == .ps_idle || player.state == .ps_walk ||
	           player.state == .ps_crouch || player.state == .ps_jump
	if any_pressed && !can_act {
		crouching := (player.state == .ps_crouch) || input.down
		if throw_pressed {
			player.buffered_attack = int(attack_type.atk_throw)
		} else {
			atk_type: attack_type
			if light_pressed {
				atk_type = crouching ? .atk_2l : .atk_5l
			} else if medium_pressed {
				atk_type = crouching ? .atk_2m : .atk_5m
			} else {
				atk_type = crouching ? .atk_2h : .atk_5h
			}
			player.buffered_attack = int(atk_type)
		}
		player.buffer_timer = INPUT_BUFFER_WINDOW
	}

	switch player.state {
	case .ps_idle, .ps_walk:
		if throw_pressed && player.input_lock == 0 {
			start_throw(player)
			break
		}
		if (light_pressed || medium_pressed || heavy_pressed) && player.input_lock == 0 {
			atk_type: attack_type
			if light_pressed {
				atk_type = .atk_5l
			} else if medium_pressed {
				atk_type = .atk_5m
			} else {
				atk_type = .atk_5h
			}
			start_attack(player, atk_type)
			break
		}

		if input.down && player.grounded {
			player.state = .ps_crouch
			break
		}

		if input.up && player.grounded {
			player.velocity_y = -player.character.jump_strength
			player.grounded = false
			player.state = .ps_jump
			break
		}

		if player.input_lock > 0 && player.velocity_x != 0.0 {
			player.velocity_x *= 0.85
			if math.abs(player.velocity_x) < 1.0 { player.velocity_x = 0.0 }
			player.state = .ps_idle
		} else {
			player.velocity_x = f32(direction_x) * player.character.walk_speed
			player.state = direction_x != 0 ? .ps_walk : .ps_idle
		}

	case .ps_crouch:
		if !input.down {
			player.state = direction_x != 0 ? .ps_walk : .ps_idle
			break
		}

		if throw_pressed && player.input_lock == 0 {
			start_throw(player)
			break
		}

		if (light_pressed || medium_pressed || heavy_pressed) && player.input_lock == 0 {
			atk_type: attack_type
			if light_pressed {
				atk_type = .atk_2l
			} else if medium_pressed {
				atk_type = .atk_2m
			} else {
				atk_type = .atk_2h
			}
			start_attack(player, atk_type)
			break
		}

		if player.input_lock > 0 && player.velocity_x != 0.0 {
			player.velocity_x *= 0.85
			if math.abs(player.velocity_x) < 1.0 { player.velocity_x = 0.0 }
		} else {
			player.velocity_x = 0
		}

	case .ps_jump:
		if throw_pressed && player.input_lock == 0 {
			start_throw(player)
			break
		}
		if (light_pressed || medium_pressed || heavy_pressed) && player.input_lock == 0 {
			atk_type: attack_type
			if light_pressed {
				atk_type = .atk_5l
			} else if medium_pressed {
				atk_type = .atk_5m
			} else {
				atk_type = .atk_5h
			}
			start_attack(player, atk_type)
			break
		}

		if player.grounded {
			player.state = direction_x != 0 ? .ps_walk : .ps_idle
		}

	case .ps_attack:
		if player.attack_type == .atk_throw {
			player.state_timer -= 1
			player.attack_frame += 1
			if player.state_timer <= 0 {
				if player.grounded {
					player.state = input.down ? .ps_crouch : (direction_x != 0 ? .ps_walk : .ps_idle)
				} else {
					player.state = .ps_jump
				}
			}
			break
		}
		atk_data := player.wakeup_attack ? &getup_attack_data : &player.character.attacks[player.attack_type]
		total_frames := atk_data.startup + atk_data.active + atk_data.recovery
		elapsed := total_frames - player.state_timer

		if !player.wakeup_attack && elapsed < atk_data.startup && throw_pressed {
			start_throw(player)
			break
		}

		if elapsed >= atk_data.startup && elapsed < atk_data.startup + atk_data.active {
			if atk_data.move_x != 0.0 {
				player.velocity_x = player.facing_right ? atk_data.move_x : -atk_data.move_x
			}
		} else if player.grounded {
			player.velocity_x = 0.0
		}

		player.state_timer -= 1
		player.attack_frame += 1
		if player.state_timer <= 0 {
			player.wakeup_attack = false
			if player.grounded {
				player.state = input.down ? .ps_crouch : (direction_x != 0 ? .ps_walk : .ps_idle)
			} else {
				player.state = .ps_jump
			}
		}

	case .ps_hitstun, .ps_blockstun:
		if player.grounded {
			player.velocity_x *= 0.85
			if math.abs(player.velocity_x) < 1.0 { player.velocity_x = 0.0 }
		}

		player.state_timer -= 1

		if player.state_timer <= 0 {
			if player.grounded {
				player.state = input.down ? .ps_crouch : (direction_x != 0 ? .ps_walk : .ps_idle)
			} else {
				player.state = .ps_jump
			}
		}

	case .ps_block:
		player.stun_anim_state = input.down ? 1 : 0

	case .ps_throwing:
		player.velocity_x = 0.0
		player.velocity_y = 0.0
		player.grounded = true
		player.state_timer -= 1

	case .ps_thrown:
		player.velocity_x = 0.0
		player.velocity_y = 0.0
		player.grounded = true
		player.state_timer -= 1

	case .ps_knockdown:
		if player.grounded {
			player.velocity_x *= 0.85
			if math.abs(player.velocity_x) < 1.0 { player.velocity_x = 0.0 }
			frames_on_ground := KNOCKDOWN_GROUND_TIME - player.state_timer
			if frames_on_ground >= KNOCKDOWN_FORCED_GROUND {
				wakeup_attack_pressed := (input.light && input.medium && input.heavy) &&
				                         !(prev_input.light && prev_input.medium && prev_input.heavy)
				if wakeup_attack_pressed {
					start_wakeup_attack(player)
					break
				}
				any_roll_pressed := (light_pressed || medium_pressed || heavy_pressed) &&
				                    !(input.light && input.medium && input.heavy)
				if any_roll_pressed {
					holding_forward := (player.facing_right && input.right) || (!player.facing_right && input.left)
					holding_backward := (player.facing_right && input.left) || (!player.facing_right && input.right)
					roll_dir: f32 = player.facing_right ? 1.0 : -1.0
					roll_vel: f32 = 0.0
					if holding_forward { roll_vel = roll_dir * ROLL_SPEED }
					else if holding_backward { roll_vel = -roll_dir * ROLL_SPEED }
					player.state = .ps_rolling
					player.state_timer = ROLL_DURATION
					player.velocity_x = roll_vel
					player.velocity_y = 0.0
					player.invincible_timer = ROLL_INVINCIBLE
					break
				}
			}
			player.state_timer -= 1
			if player.state_timer <= 0 {
				player.state = .ps_wakeup
				player.state_timer = WAKEUP_DURATION
			}
		}

	case .ps_wakeup:
		player.velocity_x = 0.0
		player.state_timer -= 1
		if throw_pressed && player.input_lock == 0 {
			start_throw(player)
			break
		}
		if (light_pressed || medium_pressed || heavy_pressed) && player.input_lock == 0 {
			if input.down {
				atk_type: attack_type
				if light_pressed {
					atk_type = .atk_2l
				} else if medium_pressed {
					atk_type = .atk_2m
				} else {
					atk_type = .atk_2h
				}
				start_attack(player, atk_type)
			} else {
				atk_type: attack_type
				if light_pressed {
					atk_type = .atk_5l
				} else if medium_pressed {
					atk_type = .atk_5m
				} else {
					atk_type = .atk_5h
				}
				start_attack(player, atk_type)
			}
			break
		}
		if player.state_timer <= 0 {
			player.state = input.down ? .ps_crouch : (direction_x != 0 ? .ps_walk : .ps_idle)
		}

	case .ps_rolling:
		if (input.light && input.medium && input.heavy) &&
		   !(prev_input.light && prev_input.medium && prev_input.heavy) {
			start_wakeup_attack(player)
			break
		}
		player.velocity_x *= 0.85
		if math.abs(player.velocity_x) < 1.0 { player.velocity_x = 0.0 }
		player.state_timer -= 1
		if player.state_timer <= 0 {
			player.state = .ps_idle
		}

	case:
	}

	if player.buffered_attack >= 0 && player.buffer_timer > 0 {
		can_act = player.state == .ps_idle || player.state == .ps_walk ||
		           player.state == .ps_crouch || player.state == .ps_jump
		if can_act {
			if attack_type(player.buffered_attack) == .atk_throw {
				start_throw(player)
			} else {
				start_attack(player, attack_type(player.buffered_attack))
			}
			player.buffered_attack = -1
			player.buffer_timer = 0
		}
	}

	if !player.grounded {
		player.velocity_y += GRAVITY * DT
	}

	player.x += player.velocity_x * DT
	player.y += player.velocity_y * DT

	if player.y >= GROUND_HEIGHT {
		player.y = GROUND_HEIGHT
		player.velocity_y = 0.0
		player.grounded = true
	} else {
		player.grounded = false
	}

	half_w := player.character.width * 0.5
	if player.x < STAGE_LEFT + half_w { player.x = STAGE_LEFT + half_w }
	if player.x > STAGE_RIGHT - half_w { player.x = STAGE_RIGHT - half_w }
}

gather_inputs :: proc() -> input_state {
	input: input_state
	input.left   = rl.IsKeyDown(.A)
	input.right  = rl.IsKeyDown(.D)
	input.up     = rl.IsKeyDown(.SPACE)
	input.down   = rl.IsKeyDown(.S)
	input.light  = rl.IsKeyDown(.J)
	input.medium = rl.IsKeyDown(.K)
	input.heavy  = rl.IsKeyDown(.L)
	return input
}

gather_inputs_p2 :: proc() -> input_state {
	input: input_state
	input.left   = rl.IsKeyDown(.LEFT)
	input.right  = rl.IsKeyDown(.RIGHT)
	input.up     = rl.IsKeyDown(.UP)
	input.down   = rl.IsKeyDown(.DOWN)
	input.light  = rl.IsKeyDown(.COMMA)
	input.medium = rl.IsKeyDown(.PERIOD)
	input.heavy  = rl.IsKeyDown(.SLASH)
	return input
}

input_state_eq :: proc(a, b: ^input_state) -> bool {
	return a^ == b^
}

run_one_sim_step :: proc(gs: ^game_state) {
	frame_input: game_input

	frame_input.players[0] = gather_inputs()
	frame_input.players[1] = gather_inputs_p2()

	index := gs.current_frame % INPUT_BUFFER_SIZE
	gs.input_buffer[index] = frame_input

	for pl in 0..<MAX_PLAYERS {
		cur := &frame_input.players[pl]
		history_entry: ^[INPUT_HISTORY_SIZE]input_history_entry
		count: ^int
		if pl == 0 {
			history_entry = &history_p1
			count = &history_count_p1
		} else {
			history_entry = &history_p2
			count = &history_count_p2
		}
		if count^ > 0 && input_state_eq(&history_entry[0].state, cur) {
			history_entry[0].duration += 1
		} else {
			if count^ < INPUT_HISTORY_SIZE { count^ += 1 }
			for h := count^ - 1; h > 0; h -= 1 {
				history_entry[h] = history_entry[h - 1]
			}
			history_entry[0].state = cur^
			history_entry[0].duration = 1
		}
	}

	empty_frame: game_input
	prev_frame: ^game_input = &empty_frame

	if gs.current_frame > 0 {
		prev_index := (gs.current_frame - 1) % INPUT_BUFFER_SIZE
		prev_frame = &gs.input_buffer[prev_index]
	}

	step_start := rl.GetTime()

	for i in 0..<MAX_PLAYERS {
		opponent := i == 0 ? 1 : 0
		game_step(&gs.players[i],
		          &gs.players[opponent],
		          &gs.input_buffer[index].players[i],
		          &prev_frame.players[i])
	}

	slot := gs.current_frame % STATE_HISTORY_SIZE
	for i in 0..<MAX_PLAYERS {
		p := &gs.players[i]
		ds: display_state
		#partial switch p.state {
		case .ps_idle:   ds = .ds_idle
		case .ps_walk:   ds = .ds_walk
		case .ps_jump:   ds = .ds_jump
		case .ps_hitstun:   ds = .ds_hitstun
		case .ps_blockstun: ds = .ds_blockstun
		case .ps_throwing:  ds = .ds_throwing
		case .ps_thrown:    ds = .ds_thrown
		case .ps_knockdown:
			frames_on_ground := 180 - p.state_timer
			ds = frames_on_ground >= 30 ? .ds_knockdown : .ds_knockdown_forced
		case .ps_wakeup:    ds = .ds_wakeup
		case .ps_rolling:   ds = .ds_rolling
		case .ps_attack:
			if p.attack_type == .atk_throw {
				ds = .ds_attack_startup
				break
			}
			atk_data := p.wakeup_attack ? &getup_attack_data : &p.character.attacks[p.attack_type]
			elapsed := p.attack_frame - 1
			if elapsed < atk_data.startup {
				ds = .ds_attack_startup
			} else if elapsed < atk_data.startup + atk_data.active {
				ds = .ds_attack_active
			} else {
				ds = .ds_attack_recovery
			}
		case:
			ds = .ds_idle
		}
		if i == 0 { state_history_p1[slot] = ds }
		else { state_history_p2[slot] = ds }
	}

	any_action := (state_history_p1[slot] >= .ds_attack_startup && state_history_p1[slot] <= .ds_thrown) ||
	              (state_history_p2[slot] >= .ds_attack_startup && state_history_p2[slot] <= .ds_thrown)

	if any_action {
		if !frame_meter_active {
			frame_meter_count = 0
			meter_p1_attack_type = -1
			meter_p2_attack_type = -1
			frame_meter_active = true
		}
		if frame_meter_count < FRAME_METER_MAX {
			frame_meter[frame_meter_count].p1 = state_history_p1[slot]
			frame_meter[frame_meter_count].p2 = state_history_p2[slot]
			frame_meter_count += 1
		}
		frame_meter_has_data = true

		if gs.players[0].state == .ps_attack { meter_p1_attack_type = int(gs.players[0].attack_type) }
		if gs.players[1].state == .ps_attack { meter_p2_attack_type = int(gs.players[1].attack_type) }
	} else {
		frame_meter_active = false
	}

	hit_results: [MAX_PLAYERS]hit_result

	for i in 0..<MAX_PLAYERS {
		attacker := &gs.players[i]
		if attacker.state != .ps_attack || attacker.attack_type != .atk_throw { continue }
		o := i == 0 ? 1 : 0
		defender := &gs.players[o]

		td := &attacker.character.throw_data
		total := td.startup + td.active + td.whiff_recovery
		elapsed := total - attacker.state_timer

		if elapsed >= td.startup && elapsed < td.startup + td.active {
			dx := math.abs(attacker.x - defender.x)
			if dx <= td.range && defender.state != .ps_throwing && defender.state != .ps_thrown {
				attacker.state = .ps_throwing
				attacker.state_timer = td.throw_duration
				attacker.velocity_x = 0.0
				attacker.velocity_y = 0.0

				defender.state = .ps_thrown
				defender.state_timer = td.throw_duration
				defender.velocity_x = 0.0
				defender.velocity_y = 0.0
			}
		}
	}

	for i in 0..<MAX_PLAYERS {
		opponent := i == 0 ? 1 : 0
		attacker := &gs.players[i]

		if attacker.state != .ps_attack { continue }
		if attacker.attack_type == .atk_throw { continue }

		atk_data := attacker.wakeup_attack ? &getup_attack_data : &attacker.character.attacks[attacker.attack_type]
		total := atk_data.startup + atk_data.active + atk_data.recovery
		elapsed := total - attacker.state_timer

		normal_strip_loop: for s in 0..<atk_data.strip_count {
			if attacker.hit_connected[s] { continue }
			strip := &atk_data.strips[s]
			if elapsed < strip.start_frame || elapsed > strip.end_frame { continue }

			defender := &gs.players[opponent]
			for h in 0..<strip.hitbox_count {
				ax, ay := world_rect(attacker, &strip.hitboxes[h].rect)
				aw := strip.hitboxes[h].rect.width
				ah := strip.hitboxes[h].rect.height

				defender_hurtboxes: [MAX_HURTBOXES]rect
				defender_hurtbox_count := get_active_hurtboxes(defender, defender_hurtboxes[:], MAX_HURTBOXES)
				for hb in 0..<defender_hurtbox_count {
					dx, dy := world_rect(defender, &defender_hurtboxes[hb])
					dw := defender_hurtboxes[hb].width
					dh := defender_hurtboxes[hb].height

					if defender.invincible_timer > 0 { continue }

					if aabb(ax, ay, aw, ah, dx, dy, dw, dh) {
						attacker.hit_connected[s] = true

						result := &hit_results[opponent]
						result.hit = true
						result.damage = strip.damage
						result.hitstun = strip.hitstun
						result.blockstun = strip.blockstun
						result.knockback_x = attacker.facing_right ? strip.knockback_x : -strip.knockback_x
						result.knockback_y = strip.knockback_y
						result.block_knockback_x = attacker.facing_right ? strip.block_knockback_x : -strip.block_knockback_x
						result.block_knockback_y = strip.block_knockback_y
						result.overhead = strip.overhead
						result.low = strip.low
						result.knockdown = strip.knockdown

						break normal_strip_loop
					}
				}
			}
		}
	}

	for i in 0..<MAX_PLAYERS {
		hit := &hit_results[i]
		if !hit.hit { continue }
		p := &gs.players[i]

		defender_input := &gs.input_buffer[index].players[i]
		attacker_idx := i == 0 ? 1 : 0
		dx := gs.players[attacker_idx].x - p.x

		holding_back := false
		if dx > 0 { holding_back = defender_input.left && !defender_input.right }
		else if dx < 0 { holding_back = defender_input.right && !defender_input.left }

		crouching := (p.state == .ps_crouch) || defender_input.down
		blocked := false
		if holding_back && p.state != .ps_attack {
			if crouching && !hit.overhead { blocked = true }
			else if !crouching && !hit.low { blocked = true }
		}

		if !blocked {
			other := i == 0 ? 1 : 0
			fmt.printf("Player %d hit Player %d:\n\t%.0f - %.0f = %.0f\n", other + 1, i + 1, p.health, hit.damage, p.health - hit.damage)

			p.health -= hit.damage
			p.stun_anim_state = (p.state == .ps_crouch) ? 1 : 0
			p.velocity_x = hit.knockback_x
			p.velocity_y = hit.knockback_y
			if hit.knockdown {
				p.state = .ps_knockdown
				p.state_timer = KNOCKDOWN_GROUND_TIME
				p.invincible_timer = KNOCKDOWN_INVINCIBLE
				p.grounded = false
			} else {
				p.state = .ps_hitstun
				p.state_timer = hit.hitstun
				p.wakeup_attack = false
			}

			attacker_remaining := gs.players[other].state_timer
			defender_stun := hit.hitstun
			advantage := defender_stun - attacker_remaining
			if other == 0 { meter_last_adv_p1 = advantage; meter_last_adv_p2 = -advantage }
			else { meter_last_adv_p2 = advantage; meter_last_adv_p1 = -advantage }
		} else {
			other := i == 0 ? 1 : 0
			fmt.printf("Player %d blocked!\n", other + 1)

			p.stun_anim_state = (p.state == .ps_crouch) ? 1 : 0
			p.state = .ps_blockstun
			p.state_timer = hit.blockstun
			p.velocity_x = hit.block_knockback_x
			p.velocity_y = hit.block_knockback_y

			attacker_remaining := gs.players[other].state_timer
			defender_stun := hit.blockstun
			advantage := defender_stun - attacker_remaining
			if other == 0 { meter_last_adv_p1 = advantage; meter_last_adv_p2 = -advantage }
			else { meter_last_adv_p2 = advantage; meter_last_adv_p1 = -advantage }
		}
	}

	for i in 0..<MAX_PLAYERS {
		p := &gs.players[i]
		if p.state != .ps_throwing { continue }
		o := i == 0 ? 1 : 0
		defender := &gs.players[o]
		td := &p.character.throw_data

		def_input := &gs.input_buffer[index].players[o]
		prev_def_input := &prev_frame.players[o]
		def_tech_pressed := (def_input.medium && def_input.heavy) && !(prev_def_input.medium && prev_def_input.heavy)

		in_tech_window := (defender.state == .ps_thrown && defender.state_timer >= td.throw_duration - THROW_TECH_WINDOW)

		if defender.state == .ps_thrown && def_tech_pressed && in_tech_window {
			p.state = .ps_idle
			p.velocity_x = 0.0
			p.buffered_attack = -1
			p.buffer_timer = 0
			p.input_lock = 8
			defender.state = .ps_idle
			defender.velocity_x = 0.0
			defender.buffered_attack = -1
			defender.buffer_timer = 0
			defender.input_lock = 8
			push_dir: f32 = p.facing_right ? -350.0 : 350.0
			p.velocity_x = push_dir
			defender.velocity_x = -push_dir
		} else if defender.state == .ps_thrown && p.state_timer <= 0 {
			defender.health -= td.damage
			defender.stun_anim_state = 0
			defender.velocity_x = p.facing_right ? td.knockback_x : -td.knockback_x
			defender.velocity_y = td.knockback_y
			if td.knockdown {
				defender.state = .ps_knockdown
				defender.state_timer = KNOCKDOWN_GROUND_TIME
				defender.invincible_timer = KNOCKDOWN_INVINCIBLE
				defender.grounded = false
			} else {
				defender.state = .ps_hitstun
				defender.state_timer = td.hitstun
				defender.wakeup_attack = false
			}

			p.state = .ps_attack
			p.attack_type = .atk_throw
			p.state_timer = td.throw_recovery
			p.velocity_x = 0.0
			p.velocity_y = 0.0
		} else if defender.state != .ps_thrown {
			p.state = .ps_idle
		}
	}

	resolve_player_collision(&gs.players[0], &gs.players[1])

	for i in 0..<MAX_PLAYERS {
		p := &gs.players[i]
		half_w := p.character.width * 0.5
		if p.x < STAGE_LEFT + half_w { p.x = STAGE_LEFT + half_w }
		if p.x > STAGE_RIGHT - half_w { p.x = STAGE_RIGHT - half_w }
		if p.y > GROUND_HEIGHT { p.y = GROUND_HEIGHT }
	}

	dx := gs.players[0].x - gs.players[1].x
	dist := math.abs(dx)
	if dist > MAX_DISTANCE {
		correction := (dist - MAX_DISTANCE) / 2.0
		if dx > 0 {
			gs.players[0].x -= correction
			gs.players[1].x += correction
		} else {
			gs.players[0].x += correction
			gs.players[1].x -= correction
		}
	}

	gs.current_frame += 1
	step_end := rl.GetTime()
	sim_frame_time_ms = f32((step_end - step_start) * 1000.0)
}

draw_health_bars :: proc(gs: ^game_state, show_numbers: bool) {
	bar_width: i32 = 200
	bar_height: i32 = 20
	bar_y: i32 = 10

	p1_bar_x: i32 = 20
	p2_bar_x: i32 = SCREEN_WIDTH - 20 - bar_width

	for i in 0..<MAX_PLAYERS {
		p := &gs.players[i]
		ratio := p.health / p.character.max_health
		if ratio < 0.0 { ratio = 0.0 }

		fg := i == 0 ? rl.RED : rl.BLUE
		bx := i == 0 ? p1_bar_x : p2_bar_x

		rl.DrawRectangle(bx, bar_y, bar_width, bar_height, rl.DARKGRAY)
		if i == 0 {
			rl.DrawRectangle(bx + bar_width - i32(f32(bar_width) * ratio), bar_y, i32(f32(bar_width) * ratio), bar_height, fg)
		} else {
			rl.DrawRectangle(bx, bar_y, i32(f32(bar_width) * ratio), bar_height, fg)
		}
		rl.DrawRectangleLines(bx, bar_y, bar_width, bar_height, rl.WHITE)

		if show_numbers {
			rl.DrawText(rl.TextFormat("%.0f / %.0f", p.health, p.character.max_health),
			            bx + 4, bar_y + 2, 14, rl.WHITE)
		}
	}
}

draw_current_input :: proc(state: input_state, x, y: i32, is_p1: bool) {
	ds: i32 = 10
	g: i32 = 3
	cw := ds + g
	ch := ds + g
	on := is_p1 ? rl.Color{200, 50, 50, 255} : rl.Color{50, 50, 200, 255}
	off := rl.Color{160, 160, 160, 200}

	rl.DrawRectangle(x + cw, y, ds, ds, state.up ? on : off)
	rl.DrawRectangle(x, y + ch, ds, ds, state.left ? on : off)
	rl.DrawRectangle(x + cw, y + ch, ds, ds, state.down ? on : off)
	rl.DrawRectangle(x + 2 * cw, y + ch, ds, ds, state.right ? on : off)

	bx := x + 3 * cw + 6
	by := y + ch - 6
	bs: i32 = 12
	bg: i32 = 4
	dim := rl.Color{80, 80, 80, 200}
	rl.DrawRectangleLines(bx, by, bs, bs, state.light ? rl.PINK : dim)
	rl.DrawText("L", bx + 2, by + 1, 9, state.light ? rl.PINK : dim)
	rl.DrawRectangleLines(bx + bs + bg, by, bs, bs, state.medium ? rl.GREEN : dim)
	rl.DrawText("M", bx + bs + bg + 2, by + 1, 9, state.medium ? rl.GREEN : dim)
	rl.DrawRectangleLines(bx + 2 * (bs + bg), by, bs, bs, state.heavy ? rl.RED : dim)
	rl.DrawText("H", bx + 2 * (bs + bg) + 2, by + 1, 9, state.heavy ? rl.RED : dim)
}

draw_input_history :: proc(entries: []input_history_entry, count: int, x, y: i32) {
	for i in 0..<count {
		s := &entries[i].state
		ry := y + i32(i) * 18
		ds: i32 = 6
		g: i32 = 2
		cw := ds + g
		ch := ds + g
		on := rl.Color{180, 180, 50, 255}
		off := rl.Color{120, 120, 120, 200}

		rl.DrawRectangle(x + cw, ry, ds, ds, s.up ? on : off)
		rl.DrawRectangle(x, ry + ch, ds, ds, s.left ? on : off)
		rl.DrawRectangle(x + cw, ry + ch, ds, ds, s.down ? on : off)
		rl.DrawRectangle(x + 2 * cw, ry + ch, ds, ds, s.right ? on : off)

		tx := x + 3 * cw + 4 + 2
		dim := rl.Color{80, 80, 80, 200}
		rl.DrawText("L", tx, ry, 9, s.light ? rl.PINK : dim); tx += 11
		rl.DrawText("M", tx, ry, 9, s.medium ? rl.GREEN : dim); tx += 11
		rl.DrawText("H", tx, ry, 9, s.heavy ? rl.RED : dim); tx += 11

		rl.DrawText(rl.TextFormat("%df", entries[i].duration), tx, ry, 9, rl.BLACK)
	}
}

display_color :: proc(s: display_state) -> rl.Color {
	switch s {
	case .ds_idle:             return rl.Color{180, 180, 180, 255}
	case .ds_walk:             return rl.Color{ 80, 130, 255, 255}
	case .ds_jump:             return rl.Color{ 80, 220,  80, 255}
	case .ds_attack_startup:   return rl.Color{255, 255,  80, 255}
	case .ds_attack_active:    return rl.Color{255,  50,  50, 255}
	case .ds_attack_recovery:  return rl.Color{180,  50,  50, 255}
	case .ds_hitstun:          return rl.Color{255, 180,  40, 255}
	case .ds_blockstun:        return rl.Color{200,  80, 255, 255}
	case .ds_throwing:         return rl.Color{255, 100, 255, 255}
	case .ds_thrown:           return rl.Color{255,  50, 200, 255}
	case .ds_knockdown:        return rl.Color{180,  50,  50, 255}
	case .ds_knockdown_forced: return rl.Color{120,  30,  30, 255}
	case .ds_wakeup:           return rl.Color{100, 200, 100, 255}
	case .ds_rolling:          return rl.Color{100, 150, 255, 255}
	case:
		return rl.Color{100, 100, 100, 255}
	}
}

advantage_color :: proc(advantage: int) -> rl.Color {
	if advantage > 0 { return rl.Color{50, 100, 255, 255} }
	if advantage < 0 { return rl.Color{255, 50, 50, 255} }
	return rl.BLACK
}

draw_advantage_line :: proc(x: i32, y: i32, pl: int, at: int, advantage: int) {
	label_x := x
	if at >= 0 {
		atk_data := &character_1.attacks[attack_type(at)]
		text := rl.TextFormat("startup %df  active %df  recovery %df  Adv: ",
		                       atk_data.startup, atk_data.active, atk_data.recovery)
		rl.DrawText(text, label_x, y, 12, rl.BLACK)
		label_x += rl.MeasureText(text, 12)
	} else {
		rl.DrawText("startup --f  active --f  recovery --f  Adv: ", label_x, y, 12, rl.BLACK)
		label_x += rl.MeasureText("startup --f  active --f  recovery --f  Adv: ", 12)
	}
	rl.DrawText(rl.TextFormat("%+df", advantage), label_x, y, 12, advantage_color(advantage))
}

draw_frame_meter :: proc() {
	bar_area_w: i32 = SCREEN_WIDTH - 90
	bar_h: i32 = 10
	gap: i32 = 3
	num_bars := FRAME_METER_MAX

	block_w := f32(bar_area_w) / f32(num_bars)
	if block_w < 4.0 { block_w = 4.0 }
	if block_w > 14.0 { block_w = 14.0 }
	actual_w := i32(block_w * f32(num_bars))
	bar_x := (SCREEN_WIDTH - actual_w) / 2

	p1_at := frame_meter_count > 0 ? meter_p1_attack_type : -1
	p2_at := frame_meter_count > 0 ? meter_p2_attack_type : -1

	p1_label_y: i32 = 122
	draw_advantage_line(bar_x, p1_label_y, 1, p1_at, meter_last_adv_p1)

	bar_y: i32 = p1_label_y + 14

	for p in 0..<num_bars {
		x := bar_x + i32(f32(p) * block_w)
		bw := i32(f32(p + 1) * block_w) - i32(f32(p) * block_w)
		if bw < 1 { bw = 1 }

		buf_idx: int
		if p < frame_meter_count {
			if frame_meter_count < FRAME_METER_MAX {
				buf_idx = p
			} else {
				buf_idx = (frame_meter_count + p) % FRAME_METER_MAX
			}
		} else {
			buf_idx = -1
		}

		c1, c2: rl.Color
		if buf_idx >= 0 {
			c1 = display_color(frame_meter[buf_idx].p1)
			c2 = display_color(frame_meter[buf_idx].p2)
		} else {
			c1 = rl.Color{180, 180, 180, 80}
			c2 = rl.Color{180, 180, 180, 80}
		}

		rl.DrawRectangle(x, bar_y, bw, bar_h, c1)
		rl.DrawRectangle(x, bar_y + bar_h + gap, bw, bar_h, c2)
	}

	rl.DrawRectangleLines(bar_x, bar_y, actual_w, bar_h, rl.BLACK)
	rl.DrawRectangleLines(bar_x, bar_y + bar_h + gap, actual_w, bar_h, rl.BLACK)

	rl.DrawText("P1", bar_x - 26, bar_y + 1, 12, rl.BLACK)
	rl.DrawText("P2", bar_x - 26, bar_y + bar_h + gap + 1, 12, rl.BLACK)

	p2_label_y: i32 = bar_y + bar_h + gap + bar_h + 6
	draw_advantage_line(bar_x, p2_label_y, 2, p2_at, -meter_last_adv_p1)
}

main :: proc() {
	rl.InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cole's Fighting Game")
	defer rl.CloseWindow()

	target := rl.LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT)
	defer rl.UnloadRenderTexture(target)

	stage_center: f32 = (STAGE_LEFT + STAGE_RIGHT) / 2.0
	init_player(&gs.players[0], stage_center - SCREEN_WIDTH * 0.25, GROUND_HEIGHT, &character_1)
	init_player(&gs.players[1], stage_center + SCREEN_WIDTH * 0.25, GROUND_HEIGHT, &character_1)

	accumulator: f64 = 0.0
	fps_timer: f64 = 0.0
	steps_this_second: int = 0

	paused: int = 0
	do_step: int = 0
	show_debug_overlay: int = 1

	camera := rl.Camera2D{
		offset = rl.Vector2{SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0},
		target = rl.Vector2{(gs.players[0].x + gs.players[1].x) / 2, SCREEN_HEIGHT / 2.0},
		zoom = 1.0,
		rotation = 0.0,
	}

	for !rl.WindowShouldClose() {
		frame_time := f64(rl.GetFrameTime())
		if frame_time > 0.1 { frame_time = 0.1 }
		render_frame_time_ms := f32(frame_time * 1000.0)

		if rl.IsKeyPressed(.F1) { show_debug_overlay = 1 - show_debug_overlay }
		if rl.IsKeyPressed(.F2) {
			stage_center = (STAGE_LEFT + STAGE_RIGHT) / 2.0
			init_player(&gs.players[0], stage_center - SCREEN_WIDTH * 0.25, GROUND_HEIGHT, &character_1)
			init_player(&gs.players[1], stage_center + SCREEN_WIDTH * 0.25, GROUND_HEIGHT, &character_1)
		}
		if rl.IsKeyPressed(.F3) { paused = 1 - paused }
		if rl.IsKeyPressed(.F4) { do_step = 1 }
		if rl.IsKeyPressed(.F5) { show_input_p1 = !show_input_p1 }
		if rl.IsKeyPressed(.F6) { show_input_p2 = !show_input_p2 }
		if rl.IsKeyPressed(.F7) { show_state_timeline = !show_state_timeline }

		if paused != 0 {
			if do_step != 0 {
				run_one_sim_step(&gs)
				do_step = 0
			}
		} else {
			accumulator += frame_time

			steps_run: int = 0
			for accumulator >= DT {
				run_one_sim_step(&gs)
				accumulator -= DT
				steps_run += 1
			}

			steps_this_second += steps_run
			fps_timer += frame_time
			if fps_timer >= 1.0 {
				sim_fps = f32(steps_this_second)
				steps_this_second = 0
				fps_timer -= 1.0
			}
		}

		midpoint_x := (gs.players[0].x + gs.players[1].x) / 2.0

		base_distance: f32 = SCREEN_WIDTH * 0.5
		distance := math.abs(gs.players[0].x - gs.players[1].x)
		desired_zoom: f32 = math.min(1.0, base_distance / distance)
		desired_zoom = math.max(desired_zoom, 0.625)

		desired_target := rl.Vector2{midpoint_x, 450.0 - 210.0 / desired_zoom}

		smooth_speed: f32 = 2.0
		t := 1.0 - math.exp(-smooth_speed * rl.GetFrameTime())
		camera.target.x += (desired_target.x - camera.target.x) * t
		camera.target.y += (desired_target.y - camera.target.y) * t
		camera.zoom += (desired_zoom - camera.zoom) * t

		view_half := (SCREEN_WIDTH / 2.0) / camera.zoom
		min_target := STAGE_LEFT + view_half
		max_target := STAGE_RIGHT - view_half

		if min_target < max_target {
			camera.target.x = math.min(math.max(camera.target.x, min_target), max_target)
		} else {
			camera.target.x = (STAGE_LEFT + STAGE_RIGHT) / 2.0
		}

		rl.BeginDrawing()
		defer rl.EndDrawing()

		rl.ClearBackground(rl.BLACK)

		scale := math.min(f32(rl.GetScreenWidth()) / SCREEN_WIDTH,
		                  f32(rl.GetScreenHeight()) / SCREEN_HEIGHT)
		vx := i32((f32(rl.GetScreenWidth()) - SCREEN_WIDTH * scale) / 2.0)
		vy := i32((f32(rl.GetScreenHeight()) - SCREEN_HEIGHT * scale) / 2.0)
		vw := i32(SCREEN_WIDTH * scale)
		vh := i32(SCREEN_HEIGHT * scale)

		rl.BeginTextureMode(target)
		defer rl.EndTextureMode()

		rl.ClearBackground(rl.RAYWHITE)
		rl.BeginMode2D(camera)
		defer rl.EndMode2D()

		rl.DrawRectangle(i32(STAGE_LEFT), i32(GROUND_HEIGHT), i32(STAGE_RIGHT - STAGE_LEFT), i32(SCREEN_HEIGHT), rl.GRAY)

		pw0 := gs.players[0].character.width
		ph0 := gs.players[0].character.height
		rl.DrawRectangle(i32(gs.players[0].x - pw0 * 0.5), i32(gs.players[0].y - ph0), i32(pw0), i32(ph0), rl.RED)
		pw1 := gs.players[1].character.width
		ph1 := gs.players[1].character.height
		rl.DrawRectangle(i32(gs.players[1].x - pw1 * 0.5), i32(gs.players[1].y - ph1), i32(pw1), i32(ph1), rl.BLUE)

		rl.EndMode2D()

		draw_health_bars(&gs, show_debug_overlay != 0)

		if paused != 0 {
			rl.DrawText("PAUSED", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 10, 30, rl.RED)
		}

		if show_debug_overlay != 0 {
			rl.BeginMode2D(camera)
			defer rl.EndMode2D()

			for pi in 0..<MAX_PLAYERS {
				player := &gs.players[pi]
				character := player.character

				hurtboxes: [MAX_HURTBOXES]rect
				hurtbox_count := get_active_hurtboxes(player, hurtboxes[:], MAX_HURTBOXES)
				for h in 0..<hurtbox_count {
					r := &hurtboxes[h]
					hx := player.x + (player.facing_right ? r.offset_x : -r.offset_x - r.width)
					hy := player.y + r.offset_y
					rl.DrawRectangleLines(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{0, 255, 0, 255})
					rl.DrawRectangle(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{0, 255, 0, 80})
				}

				boxes: [MAX_COLLISION_BOXES]collision_box
				box_count := get_active_collision_boxes(player, boxes[:], MAX_COLLISION_BOXES)
				for b in 0..<box_count {
					r := &boxes[b].rect
					hx, hy := world_rect(player, r)
					rl.DrawRectangleLines(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{255, 255, 0, 255})
					rl.DrawRectangle(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{255, 255, 0, 60})
				}

				if player.state == .ps_attack {
					atk_data := player.wakeup_attack ? &getup_attack_data : &character.attacks[player.attack_type]
					total_frames := atk_data.startup + atk_data.active + atk_data.recovery
					elapsed := total_frames - player.state_timer
					for s in 0..<atk_data.strip_count {
						strip := &atk_data.strips[s]
						if elapsed >= strip.start_frame && elapsed <= strip.end_frame {
							for b in 0..<strip.hitbox_count {
								r := &strip.hitboxes[b].rect
								hx := player.x + (player.facing_right ? r.offset_x : -r.offset_x - r.width)
								hy := player.y + r.offset_y
								rl.DrawRectangleLines(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{255, 0, 0, 255})
								rl.DrawRectangle(i32(hx), i32(hy), i32(r.width), i32(r.height), rl.Color{255, 0, 0, 80})
							}
						}
					}
				}
			}
		}

		rl.BeginTextureMode(target)

		if show_debug_overlay != 0 {
			rl.DrawText(rl.TextFormat("Frame: %d", gs.current_frame), 10, 45, 20, rl.BLACK)
			rl.DrawText(rl.TextFormat("Sim FPS: %.0f", sim_fps), 10, 65, 20, rl.BLACK)
			rl.DrawText(rl.TextFormat("Sim Frame Time: %.3f ms", sim_frame_time_ms), 10, 85, 20, rl.BLACK)
			rl.DrawText(rl.TextFormat("Render Frame Time: %.3f ms", render_frame_time_ms), 10, 105, 20, rl.BLACK)

			if show_state_timeline {
				draw_frame_meter()
			}

			input_y: i32 = SCREEN_HEIGHT - 10
			if show_input_p1 {
				idx := (gs.current_frame + INPUT_BUFFER_SIZE - 1) % INPUT_BUFFER_SIZE
				draw_current_input(gs.input_buffer[idx].players[0], 10, input_y - 40, true)
				draw_input_history(history_p1[:], history_count_p1, 10, input_y - 40 - 18 * i32(history_count_p1) - 5)
			}
			if show_input_p2 {
				idx := (gs.current_frame + INPUT_BUFFER_SIZE - 1) % INPUT_BUFFER_SIZE
				p2x: i32 = SCREEN_WIDTH - 10 - 90
				draw_current_input(gs.input_buffer[idx].players[1], p2x, input_y - 40, false)
				draw_input_history(history_p2[:], history_count_p2, p2x, input_y - 40 - 18 * i32(history_count_p2) - 5)
			}

			rl.DrawText("[F1] Debug  [F2] Reset  [F3] Pause  [F4] Step  [F5] P1 In  [F6] P2 In  [F7] Timeline",
			            10, SCREEN_HEIGHT - 20, 12, rl.BLACK)
		}

		rl.EndTextureMode()

		rl.DrawTexturePro(target.texture,
		                  rl.Rectangle{0, 0, SCREEN_WIDTH, -SCREEN_HEIGHT},
		                  rl.Rectangle{f32(vx), f32(vy), f32(vw), f32(vh)},
		                  rl.Vector2{0, 0}, 0.0, rl.WHITE)
	}
}
