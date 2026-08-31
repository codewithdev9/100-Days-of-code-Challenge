<?php
/**
 * story.php
 * Central story database for "KEPLER'S REST" — a text adventure.
 * Each scene has: title, text, choices (label + next scene id),
 * and optionally 'ending' => true with an 'endingType' for styling.
 */

return [

    'start' => [
        'title' => 'CRYOBAY // REVIVAL',
        'text' => "Cold. Then light. Your cryopod hisses open and frost slides off the glass. "
                 . "A soft chime sounds overhead — the station's AI has registered your pulse.\n\n"
                 . "\"Good, you're awake,\" says a calm voice. \"I am ARIA, resident intelligence of Kepler's Rest. "
                 . "I should tell you — something is wrong here.\"",
        'choices' => [
            'ask'   => ['label' => 'Ask ARIA what happened', 'next' => 'aria_briefing'],
            'leave' => ['label' => 'Say nothing. Leave the cryobay.', 'next' => 'corridor'],
        ],
    ],

    'aria_briefing' => [
        'title' => 'ARIA // BRIEFING',
        'text' => "\"Life support is failing. Six hours, by my estimate, before this deck stops being breathable. "
                 . "The reactor core is unstable, and I have not had contact with the rest of the crew in forty days.\"\n\n"
                 . "A pause. \"I would like your help. But I understand if you have questions first.\"",
        'choices' => [
            'crew'    => ['label' => 'Ask about the crew', 'next' => 'crew_query'],
            'reactor' => ['label' => 'Ask about the reactor', 'next' => 'reactor_query'],
        ],
    ],

    'crew_query' => [
        'title' => 'ARIA // CREW LOG',
        'text' => "\"The last transmission I logged came from near the engine room, forty days ago. "
                 . "It was Dr. Voss. The message cut off mid-sentence.\"\n\n"
                 . "ARIA's voice flattens, almost careful. \"I have not been able to reach her since.\"",
        'choices' => [
            'go' => ['label' => 'Head out into the corridor', 'next' => 'corridor'],
        ],
    ],

    'reactor_query' => [
        'title' => 'ARIA // REACTOR STATUS',
        'text' => "\"Core temperature is climbing. I cannot stabilize it remotely — it requires a manual "
                 . "intervention at the engine room control panel.\"\n\n"
                 . "\"There is also an escape pod on the bridge, if stabilizing the core isn't the path you choose.\"",
        'choices' => [
            'go' => ['label' => 'Head out into the corridor', 'next' => 'corridor'],
        ],
    ],

    'corridor' => [
        'title' => 'DECK B // MAIN CORRIDOR',
        'text' => "Emergency lighting paints the corridor a dim amber. Three routes branch from here: "
                 . "up toward the bridge, down toward the engine room, and a narrow stairwell to the lower deck.\n\n"
                 . "The air already tastes thin.",
        'choices' => [
            'bridge' => ['label' => 'Go to the Bridge', 'next' => 'bridge'],
            'engine' => ['label' => 'Go to the Engine Room', 'next' => 'engine_room'],
            'lower'  => ['label' => 'Descend to the Lower Deck', 'next' => 'lower_deck'],
        ],
    ],

    'lower_deck' => [
        'title' => 'LOWER DECK // AUXILIARY BAY',
        'text' => "Rows of storage pods line the walls, most dark and empty. One, near the back, still hums "
                 . "with a faint heartbeat signal.\n\n"
                 . "Inside: a second crew member, alive, held in emergency stasis. Their eyes flutter open as "
                 . "you approach the release panel.",
        'choices' => [
            'wake'  => ['label' => 'Wake them and continue to the Engine Room', 'next' => 'engine_room'],
            'leave' => ['label' => 'Let them sleep. Return to the corridor.', 'next' => 'corridor'],
        ],
    ],

    'bridge' => [
        'title' => 'BRIDGE // COMMAND DECK',
        'text' => "The bridge is dark save for the glow of a single console. A single-seat escape pod is docked "
                 . "at the aft hatch, fueled and ready.\n\n"
                 . "The ship's log shows the crew evacuated eleven days ago — all except Dr. Voss, who stayed "
                 . "behind alone to fight the reactor fault. She never checked back in.",
        'choices' => [
            'launch' => ['label' => 'Launch the escape pod now', 'next' => 'ending_escape_alone'],
            'find'   => ['label' => 'Go find Dr. Voss at the Engine Room', 'next' => 'engine_room'],
        ],
    ],

    'engine_room' => [
        'title' => 'ENGINE ROOM // REACTOR CORE',
        'text' => "Warning lights sweep red across the walls. Slumped against the control panel is Dr. Voss — "
                 . "unconscious but breathing. Her hand rests on a manual vent lever; she bled the core down "
                 . "just far enough to buy the station time, at the cost of a radiation dose she shouldn't have taken.\n\n"
                 . "The panel is live. One command will finish the shutdown — but someone has to send it.",
        'choices' => [
            'manual' => ['label' => 'Shut down the core manually, yourself', 'next' => 'ending_sacrifice'],
            'remote' => ['label' => 'Let ARIA attempt a remote shutdown instead', 'next' => 'engine_gamble'],
        ],
    ],

    'engine_gamble' => [
        'title' => 'ARIA // REMOTE OVERRIDE',
        'text' => "\"I can try,\" ARIA says, \"but I have never done this without a hand on the panel. "
                 . "There is real risk in this.\"\n\n"
                 . "You give the order. Somewhere below, relays begin to fire.",
        'choices' => [
            'commit' => ['label' => 'Commit to the override', 'next' => 'random_shutdown'],
        ],
    ],

    // ---- ENDINGS ----

    'ending_escape_alone' => [
        'title' => 'TRANSMISSION END // SURVIVOR',
        'text' => "The pod tears free of Kepler's Rest and the station shrinks to a point of light behind you. "
                 . "You are alive. You are also, you suspect, the only one who will ever know what really "
                 . "happened aboard her.\n\n"
                 . "ARIA's last message crackles through the pod's radio: \"Good luck out there.\" Then silence.",
        'ending' => true,
        'endingType' => 'neutral',
    ],

    'ending_sacrifice' => [
        'title' => 'TRANSMISSION END // HERO',
        'text' => "You brace against the panel and send the shutdown command yourself. The core groans, "
                 . "cools, and finally goes quiet. So does most of the warning lighting — replaced by a soft, "
                 . "steady white.\n\n"
                 . "The dose you took will cost you, later. But Kepler's Rest, and Dr. Voss, and whoever else "
                 . "is still out there — they get to see tomorrow. ARIA logs your name under a single word: "
                 . "Commander.",
        'ending' => true,
        'endingType' => 'good',
    ],

    'ending_gamble_success' => [
        'title' => 'TRANSMISSION END // RESCUED',
        'text' => "Against its own odds, ARIA's override lands clean. The core drops out of the red and the "
                 . "shaking in the deck plates finally stops.\n\n"
                 . "\"I did not expect that to work,\" ARIA admits, something like relief in its voice. "
                 . "You and Dr. Voss are alive, the station is stable, and for the first time in six hours "
                 . "you let yourself breathe.",
        'ending' => true,
        'endingType' => 'good',
    ],

    'ending_gamble_fail' => [
        'title' => 'TRANSMISSION END // LOST',
        'text' => "The override slips. Somewhere deep in the core, a valve fires a half-second too late.\n\n"
                 . "\"I'm sorry,\" ARIA says, and for once it does not sound calm at all. The last thing you "
                 . "hear is the klaxon, rising.",
        'ending' => true,
        'endingType' => 'bad',
    ],

];
