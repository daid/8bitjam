showMessage("There he is! Let's get him!", "Hero", 1)
moveCursor(HeroAI.x, HeroAI.y)
while messageOpen() do yield() end
showMessage("Enemies are here!\nWhat should I do?\nPANIC!", "Hoplite", 0)
moveCursor(HoplitePlayer.x, HoplitePlayer.y)
while messageOpen() do
    for n=1, 15 do yield() end
    HoplitePlayer.move(0, 2)
    yield()
    while HoplitePlayer.isMoving() do yield() end
    for n=1, 15 do yield() end
    HoplitePlayer.move(1, 2)
    while HoplitePlayer.isMoving() do yield() end
    yield()
end

a = getUnitAt(-1, 7)
b = getUnitAt(2, 7)
a.move(0, 3)
b.move(1, 3)
while a.isMoving() or b.isMoving() do yield() end

a = getUnitAt(-2, 7)
b = getUnitAt(3, 7)
a.move(0, 4)
b.move(1, 4)
while a.isMoving() or b.isMoving() do yield() end
showMessage("We are here to help!", "Hero", 0)
moveCursor(a.x, a.y)
