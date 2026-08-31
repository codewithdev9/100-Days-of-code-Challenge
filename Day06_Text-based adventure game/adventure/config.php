<?php
/**
 * config.php
 * Bootstraps the session and loads the story database.
 * Included by every page that needs game state.
 */

declare(strict_types=1);

if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

$story = require __DIR__ . '/story.php';
