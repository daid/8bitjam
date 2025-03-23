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
checkHero("Heracles", HeraclesPlayer, "Hero")
yield() -- update valid flags

heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Finally, a temple, a moment of reprieve.]])
AphroditePlayer.move(-1, -3)
while AphroditePlayer.isMoving() do yield() end
AphroditePlayer.move(0, -3)
while AphroditePlayer.isMoving() do yield() end
AphroditePlayer.move(0, 0)
while AphroditePlayer.isMoving() do yield() end
AphroditeAI = createUnit(0, -1, "Aphrodite", 1)
AphroditeAI.setGhost()
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
What sorcery is this!?]])
heroTalk(AphroditeAI, "Aphrodite", 1, [[Aphrodite?:
You believe yourself worthy to stand upon Mt. Olympus?]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Vile reflection, I am a GOD!]])
heroTalk(AphroditeAI, "Aphrodite", 1, [[Aphrodite?:
Hehehe... Then prove it!]])
AphroditeAI.teleport(-7, 5)
moveCursor(-7, 5)
for n=0,50 do yield() end
createUnit(-8, 6, "Cherub", 1)
createUnit(-6, 6, "Cherub", 1)
for n=0,50 do yield() end
createUnit(-3, 6, "Hero", 1)
createUnit(-5, 1, "Hoplite", 1)
createUnit(-6, 0, "Hoplite", 1)
for n=0,50 do yield() end
createUnit(6, 5, "Harpy", 1)
createUnit(5, 0, "Pirate", 1)
createUnit(3,-4, "Pirate", 1)
createUnit(6,-5, "Harpy", 1)

moveCursor(AphroditePlayer.x, AphroditePlayer.y)
showMessage([[Tutorial:
Survive for 12 turns!]], "", 0)
while messageOpen() do yield() end
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
I will not be bested by a mere mirror.]])


function onTurnEnd()
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
        ArtemisAI = createUnit(-7, -4, "Artemis", 1)
        ArtemisAI.setGhost()
        createUnit(-8, -4, "Pirate", 1)
        createUnit(-7, -5, "Pirate", 1)
        heroTalk(ArtemisAI, "Artemis", 1, [[Artemis?:
I don't miss my prey.]])
        heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
Great, now there's more of them.]])
    end
    if turn_nr == 5 then
        ApolloAI = createUnit(-7, 5, "Apollo", 1)
        ApolloAI.setGhost()
        HeraclesAI = createUnit(-8, 6, "Heracles", 1)
        HeraclesAI.setGhost()
        createUnit(-8, 5, "Hero", 1)
        createUnit(-7, 6, "Hero", 1)
        createUnit(-3, 6, "Hoplite", 1)
        createUnit(-4, 6, "Hoplite", 1)
        heroTalk(ApolloAI, "Apollo", 1, [[Apollo?:
I grace you with my light!]])
        heroTalk(HeraclesAI, "Heracles", 1, [[Heracles?:
My strength is unmatched! You stand no chance!]])
        heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
Stay strong, we will banish these vile shadows!]])
        heroTalk(HeraclesPlayer, "Heracles", 0, [[Heracles:
One who has not completed the twelve labours has no right to call themself strongest!]])
    end
    if turn_nr == 7 then
        PoseidonAI = createUnit(6, -2, "Poseidon", 1)
        PoseidonAI.setGhost()
        createUnit(6, 5, "Harpy", 1)
        createUnit(5, 0, "Pirate", 1)
        createUnit(3,-4, "Pirate", 1)
        createUnit(6,-5, "Harpy", 1)
        heroTalk(PoseidonAI, "Poseidon", 1, [[Poseidon?:
All water is my domain!]])
        heroTalk(PoseidonPlayer, "Poseidon", 0, [[Poseidon:
Hold strong! We will weather this storm together!]])
    end
    if turn_nr == 9 then
        if AphroditeAI.valid then
            if AphroditeAI.isPlayer() then
                x, y = AphroditeAI.x, AphroditeAI.y
                for n=0,10 do
                    AphroditeAI.destroy()
                    AphroditeAI = createUnit(x, y, "Aphrodite", 1)
                    for delay=0, 10 do yield() end
                    AphroditeAI.destroy()
                    AphroditeAI = createUnit(x, y, "Aphrodite", 0)
                    for delay=0, 10 do yield() end
                end
                AphroditeAI.destroy()
                AphroditeAI = createUnit(x, y, "Aphrodite", 1)
            end
            AphroditeAI.teleport(-7, 5)
            AphroditeAI.setGhost()
            heroTalk(AphroditeAI, "Aphrodite", 1, [[Aphrodite?:
Hehehe... Will you survive the full might of the Evil Eye!?]])
            createUnit(-8, 6, "Cherub", 1)
            createUnit(-3, 6, "Hero", 1)
            createUnit(-5, 1, "Hoplite", 1)
            createUnit(6, 5, "Harpy", 1)
            createUnit(3,-4, "Pirate", 1)
            heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
We've come this far, we will not fall here!]])
        end
    end
    if turn_nr == 12 then
        if not AphroditeAI.valid then
            AphroditeAI = createUnit(-7, 5, "Aphrodite", 1)
        end
        heroTalk(AphroditeAI, "Aphrodite", 1, [[Aphrodite?:
So, your resolve is this strong. You may pass to the final trial.]])
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
The final trial... He awaits us at the summit.]])
        victory()
    end
end
