<?php
require __DIR__ . '/config.php';

// --- resolve state -------------------------------------------------

if (isset($_GET['new']) || !isset($_SESSION['log'])) {
    $_SESSION['log'] = [];
}

$sceneId = $_GET['scene'] ?? 'start';

// A choice link carries the label of the choice just taken, so we can
// print a running transmission log in the sidebar.
if (isset($_GET['via']) && $_GET['via'] !== '') {
    $_SESSION['log'][] = $_GET['via'];
}

// Special routing target used by the engine_gamble scene: resolves to
// one of two endings at random when the player commits to the override.
if ($sceneId === 'random_shutdown') {
    $sceneId = (random_int(0, 1) === 1) ? 'ending_gamble_success' : 'ending_gamble_fail';
}

if (!array_key_exists($sceneId, $story)) {
    $sceneId = 'start';
}

$scene = $story[$sceneId];
$isEnding = !empty($scene['ending']);

include __DIR__ . '/header.php';
?>

<div class="term-bar">
  <span><span class="dot"></span>KEPLER'S REST // TERMINAL ACCESS</span>
  <span><?= $isEnding ? 'STATUS: TERMINATED' : 'STATUS: LIVE FEED' ?></span>
</div>

<div class="term-body">

  <div class="term-main">

    <?php if ($isEnding): ?>
      <span class="ending-badge ending-<?= htmlspecialchars($scene['endingType']) ?>">
        End of Transmission
      </span>
    <?php endif; ?>

    <div class="scene-title"><?= htmlspecialchars($scene['title']) ?></div>
    <hr class="scene-rule">
    <div class="scene-text"><?= htmlspecialchars($scene['text']) ?></div>

    <?php if (!$isEnding): ?>
      <div class="choices">
        <?php foreach ($scene['choices'] as $choice): ?>
          <a class="choice-btn"
             href="game.php?scene=<?= urlencode($choice['next']) ?>&amp;via=<?= urlencode($choice['label']) ?>">
            <?= htmlspecialchars($choice['label']) ?>
          </a>
        <?php endforeach; ?>
      </div>
    <?php else: ?>
      <a href="restart.php" class="btn btn-reboot">Reboot Terminal</a>
    <?php endif; ?>

  </div>

  <div class="term-log">
    <h6>Transmission Log</h6>
    <?php if (empty($_SESSION['log'])): ?>
      <div class="empty">No entries yet.</div>
    <?php else: ?>
      <ol>
        <?php foreach ($_SESSION['log'] as $entry): ?>
          <li><?= htmlspecialchars($entry) ?></li>
        <?php endforeach; ?>
      </ol>
    <?php endif; ?>
  </div>

</div>

<?php include __DIR__ . '/footer.php'; ?>
