function checkHero(name, unit, replacement)
    if not hasHero(name) then
        createUnit(unit.x, unit.y, replacement, 0)
        unit.destroy()
    end
end
function heroTalk(unit, name, team, message)
    print(name)
    if unit.valid then
        moveCursor(unit.x, unit.y)
        showMessage(message, name, team)
        while messageOpen() do yield() end
        return true
    end
    return false
end
function spawnUnitNear(x, y, key, team)
    print(x, y, key, getUnitAt(x, y))
    if getUnitAt(x, y) ~= nil then
        if getUnitAt(x - 1, y) == nil then x = x - 1
        elseif getUnitAt(x + 1, y) == nil then x = x + 1
        elseif getUnitAt(x, y - 1) == nil then y = y - 1
        elseif getUnitAt(x, y + 1) == nil then y = y + 1
        elseif getUnitAt(x - 1, y + 1) == nil then x = x - 1; y = y + 1
        elseif getUnitAt(x + 1, y + 1) == nil then x = x + 1; y = y + 1
        elseif getUnitAt(x - 1, y - 1) == nil then x = x - 1; y = y - 1
        elseif getUnitAt(x + 1, y - 1) == nil then x = x + 1; y = y - 1
        end
    end
    print(x, y, key)
    return createUnit(x, y, key, team)
end

checkHero("Apollo", ApolloPlayer, "Hero")
checkHero("Artemis", ArtemisPlayer, "Pirate")
checkHero("Poseidon", PoseidonPlayer, "Pirate")
yield() -- update valid flags

heroTalk(PoseidonPlayer, "Poseidon", 0, [[Poseidon:
I miss the sea already...]])
if not heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
That city is awfully defended.]]) then
    heroTalk(HeroPlayer, "Hero", 0, [[Hero:
That city is awfully defended.]])
end
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Perhaps another of the gods has made it a stronghold?]])

heroTalk(HeraclesAI, "Heracles", 1, [[Heracles:
Alright, strengthen that wall!]])

HeroAI = getUnitAt(2, 1)
moveCursor(HeroAI.x, HeroAI.y)
showMessage([[Hero:
Yes sir Mr. Heracles!]], "Hero", 1)
HeroAI.move(3, -2)
getUnitAt(4, 1).move(3, -1)
while messageOpen() do yield() end

heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Heracles? What is a hero of men doing here?]])

heroTalk(HeraclesAI, "Heracles", 1, [[Heracles:
The army we were warned about. Brace yourself!]])
heroTalk(HeraclesAI, "Heracles", 1, [[Heracles:
We will not lose this day!]])

heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
I suppose we have no choice...]])

showMessage([[Tutorial:
Defeat or Charm Heracles!]], "", 0)
while messageOpen() do yield() end

function onTurnEnd()
    if not HeraclesAI.valid or HeraclesAI.isPlayer() then
        if HeraclesAI.valid then
            heroTalk(HeraclesAI, "Heracles", 0, [[Heracles:
I see, you're answering the summons of Olympus.]])
            heroTalk(HeraclesAI, "Heracles", 0, [[Heracles:
Allow me to assist you!]])
        else
            showMessage([[Heracles:
I see, you're answering the summons of Olympus.]], "Heracles", 1)
            while messageOpen() do yield() end
            showMessage([[Heracles:
My apologies for standing in your way, oh gods.]], "Heracles", 1)
            while messageOpen() do yield() end
        end
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
We press onward!]])
        victory()
    end
end
local turn_nr = 0
function onTurnStart()
    if not AphroditePlayer.valid then
        showMessage([[Aphrodite has been defeated. You lost.]], "", 0)
        while messageOpen() do yield() end
        defeat()
        return
    end

    turn_nr = turn_nr + 1
    if turn_nr == 3 then
        heroTalk(HeraclesAI, "Heracles", 1, [[Heracles:
Men, to arms!]])
        moveCursor(AphroditePlayer.x, AphroditePlayer.y)
        if getUnitAt(-7, 4) == nil then createUnit(-7, 4, "Hoplite", 1) end
        if getUnitAt(-4, 4) == nil then createUnit(-4, 4, "Hoplite", 1) end
        if getUnitAt( 2, 3) == nil then createUnit( 2, 3, "Pirate", 1) end
        if getUnitAt( 4, 3) == nil then createUnit( 4, 3, "Pirate", 1) end
    end
end
