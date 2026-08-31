KEPLER'S REST — Text Adventure (PHP + Bootstrap)
==================================================

FILES
  index.php     Title screen — entry point
  game.php      Game engine (reads story.php, renders scenes, tracks log)
  story.php     All scene text + branching choices live here
  config.php    Starts the session, loads story.php (included by every page)
  header.php    Shared <head> + opening markup (Bootstrap + custom CSS)
  footer.php    Shared closing markup + Bootstrap JS
  restart.php   Clears the session and returns to the title screen
  css/style.css Custom CRT-terminal theme layered over Bootstrap

HOW TO RUN
  You need a local PHP server (PHP 7.4+). From inside this folder run:

      php -S localhost:8000

  Then open http://localhost:8000 in your browser.

  (Any standard PHP host / XAMPP / MAMP works too — just drop the whole
  folder in and open index.php.)

HOW IT'S WIRED TOGETHER
  index.php resets the session and links to game.php?scene=start&new=1.
  game.php reads the ?scene= id, looks it up in story.php, and renders
  its text + choice buttons. Each choice link points to game.php with
  the next scene id, so clicking through the story just moves through
  the story.php array — nothing is hardcoded into game.php itself.
  A sidebar "Transmission Log" reads from $_SESSION['log'] and grows
  every time a choice is made.

EXTENDING THE STORY
  Add a new scene to story.php as another array entry with a 'title',
  'text', and 'choices' (each with a 'label' and the 'next' scene id
  to point to). Mark an ending scene with 'ending' => true and an
  'endingType' of good / bad / neutral to color its badge.
