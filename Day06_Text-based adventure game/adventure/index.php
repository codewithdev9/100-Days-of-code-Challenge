<?php
require __DIR__ . '/config.php';
// A fresh visit to index.php always wipes any prior run.
$_SESSION['log'] = [];
include __DIR__ . '/header.php';
?>

<div class="term-bar">
  <span><span class="dot"></span>KEPLER'S REST // TERMINAL ACCESS</span>
  <span>STATUS: STANDBY</span>
</div>

<div class="splash">
  <h1>KEPLER'S REST</h1>
  <div class="tag">A Text Transmission // Interactive Log</div>
  <p class="flavor">
    Deep space station <strong>Kepler's Rest</strong> has stopped answering hails.
    Your cryopod just opened on its own. Somewhere on board, an AI called ARIA
    is still waiting for someone to wake up.
  </p>
  <a href="game.php?scene=start&amp;new=1" class="btn btn-begin">Begin Transmission</a>
</div>

<?php include __DIR__ . '/footer.php'; ?>
