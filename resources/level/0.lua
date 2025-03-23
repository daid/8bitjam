moveCursor(AphroditePlayer.x, AphroditePlayer.y)
showMessage([[Aphrodite:
Olympus calls, and the Goddess of Love answers.]], "Aphrodite", 0)
while messageOpen() do yield() end
guard = getUnitAt(2, 8)
guard.move(3, 12)
while guard.isMoving() do yield() end
showMessage([[Hoplite (Scout):
Lady Aphrodite, forces ahead!]], "Hoplite", 0)
while messageOpen() do yield() end
showMessage([[Aphrodite:
Whom bars the path of a god?]], "Aphrodite", 0)
while messageOpen() do yield() end
showMessage([[Hoplite (Scout):
From the east, Lady Artemis, and from the south Lord Apollo.]], "Hoplite", 0)
while messageOpen() do yield() end
showMessage([[Aphrodite:
Really? They must also have heeded the summons.]], "Aphrodite", 0)
while messageOpen() do yield() end
showMessage([[Aphrodite:
Pay them no heed.]], "Aphrodite", 0)
while messageOpen() do yield() end
showMessage([[Tutorial:
Select units with X and move them into position.]], "", 0)
while messageOpen() do yield() end
showMessage([[Tutorial:
Press Z while your cursor is over a unit to see their stats.]], "", 0)
while messageOpen() do yield() end
showMessage([[Tutorial:
The turn ends once you have activated all of your units.]], "", 0)
while messageOpen() do yield() end

function onTurnEnd()
    if #allUnits(1) == 0 then
        showMessage([[Aphrodite:
That's the last of them.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite:
And I doubt things are going to get easier from here.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite:
We should press forward.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite:
No doubt the other gods will be racing us to the summit.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[
VICTORY]], "", 0)
        while messageOpen() do yield() end
        victory()
    end
end

turn_number = 0
function onTurnStart()
    if not AphroditePlayer.valid then
        showMessage([[Aphrodite has been defeated. You lost.]], "", 0)
        while messageOpen() do yield() end
        defeat()
        return
    end

    turn_number = turn_number + 1
    if turn_number == 1 then
        showMessage([[Hoplite (Scout):
Lady Aphrodite, they're approaching our position!]], "Hoplite", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite:
It would appear they are making this a trial.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Hero:
Don't worry milady, I shall allow no harm to come to you!]], "Hero", 0)
        while messageOpen() do yield() end
        showMessage([[Much appreciated, but there are ways other than the sword to overcome an enemy.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Tutorial:
Each unit has a different attack.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Gods and characters of myth have the ability to Charm opponents.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite's Charm is especially potent.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Tutorial:
A unit's stats will display their Charm Hearts.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Turning all of these red will have the unit convert to your side. This will even work on opposing gods.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Tutorial:
Give it a try! Apollo and Artemis will make valuable allies!]], "", 0)
        while messageOpen() do yield() end
    end
    if turn_number == 2 then
        showMessage([[Hoplite (Scout):
We have injured, Lady Aphrodite!]], "Hoplite", 0)
        while messageOpen() do yield() end
        showMessage([[Aphrodite:
A scratch. But we can tend to it nonetheless.]], "Aphrodite", 0)
        while messageOpen() do yield() end
        showMessage([[Tutorial:
Charm can also be used on allied units to restore some HP,]], "", 0)
        while messageOpen() do yield() end
        showMessage([[which could mean all the difference in surviving an extra hit.]], "", 0)
        while messageOpen() do yield() end
        showMessage([[Consider all your options, and you will conquer the Trials of Olympus with ease!]], "", 0)
        while messageOpen() do yield() end
    end
end

--[[
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
--]]
