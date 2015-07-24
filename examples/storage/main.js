Sys=Sys1;
Prg=Prg1;

function main()
{
    var content = Prg.storage("storage/content1");
    print("Content-name: "+content.name);
    print("Content-id: "+content.id);
    print("Content-size: "+content.size);

    var buffer = Prg.storage_buffer(content.id, 0, content.size);

    print("Buffer: "+buffer);

    Prg.storage_write(1, content.id, 1, 5);
    print();
}
