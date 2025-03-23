function checkHero(name, unit, replacement)
    if not hasHero(name) then
        createUnit(unit.x, unit.y, replacement, 0)
        unit.destroy()
    end
end
function heroTalk(unit, name, team, message)
    if unit.valid then
        moveCursor(unit.x, unit.y)
        showMessage(message, name, team)
        while messageOpen() do yield() end
    end
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
yield() -- update valid flags

heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
I sense something amiss.]])
heroTalk(PoseidonAI, "Poseidon", 1, [[Poseidon:
Who dares to encroach on my seas?]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Lovely...]])
heroTalk(PoseidonAI, "Poseidon", 1, [[Poseidon:
Aphrodite? What are you doing here, my niece?]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Passing to my temple.]])
heroTalk(PoseidonAI, "Poseidon", 1, [[Poseidon:
YOUR temple by MY sea!? This is blasphemy!]])
heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
I think you made him mad.]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
I need to tend to my temple to gather my son's cherubs.]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
We'll just have to hold our own.]])
heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
You can count on me!]])

moveCursor(AphroditePlayer.x, AphroditePlayer.y)
showMessage([[Tutorial:
Hold the temple across the river for 3 turns.]], "", 0)
while messageOpen() do yield() end

hold_temple_count = 0
function onTurnEnd()
    if getUnitAt(5, 2) and getUnitAt(5, 2).isPlayer() then
        hold_temple_count = hold_temple_count + 1
    else
        hold_temple_count = 0
    end
    if hold_temple_count > 3 then
        a = spawnUnitNear(5, 2, "Cherub", 0)
        b = spawnUnitNear(5, 2, "Cherub", 0)
        moveCursor(a.x, a.y)
        showMessage([[Cherub:
Greetings, Lady Aphrodite!]], "Cherub", 0)
        while messageOpen() do yield() end
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Well met, agent of Eros. When we're done here, send my son my regards.]])
        if PoseidonAI.valid and PoseidonAI.isPlayer() then
            heroTalk(PoseidonAI, "Poseidon", 0, [[Poseidon:
My apologies for my rudeness, my niece. Let us proceed to Olympus!]])
        end
        victory()
    end
end
function onTurnStart()
    if not AphroditePlayer.valid then
        showMessage([[Aphrodite has been defeated. You lost.]], "", 0)
        while messageOpen() do yield() end
        defeat()
        return
    end
end
