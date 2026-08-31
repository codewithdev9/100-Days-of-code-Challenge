<?php
require __DIR__ . '/config.php';

// Wipe the run entirely and send the player back to the title screen.
$_SESSION = [];
if (session_status() === PHP_SESSION_ACTIVE) {
    session_destroy();
}

header('Location: index.php');
exit;
