co1 = coroutine.create(function ()
        local i = 1
        while i < 5 do
                print("coroutine 1")
                i = i + 1
        end
        coroutine.yield()
end)

co2 = coroutine.create(function ()
        local i = 1
        while i < 5 do
                print("coroutine 2")
                i = i + 1
        end
        coroutine.yield()
end)

coroutine.resume(co1)
coroutine.resume(co2)

print(os.clock())
print(os.date())
print(os.getenv("EDITOR"))
os.execute("touch __samplefile")
os.rename("__samplefile", "__samplefile2")
os.remove("__samplefile2")
os.date("%c", os.time{year = 2017, month = 01, day = 01, hour = 0, min = 0, sec = 0})
os.exit(0)

