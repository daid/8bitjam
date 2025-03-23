units = allUnits(0)
if #units > 0 then
    moveCursor(units[1].x, units[1].y);
end

function onTurnEnd()
end
function onTurnStart()
    if #allUnits(0) == 0 then
        showMessage([[You lost...]], "", 0)
        while messageOpen() do yield() end
        defeat()
    end
end