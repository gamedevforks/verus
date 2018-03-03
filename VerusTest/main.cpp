#include "stdafx.h"

using namespace verus;

void MainTests()
{
	Str::Test();
	Convert::Test();
}

void MainFile()
{
	IO::File file;
	file.Open("Testing.bin", "wb");
	file << 10;
	file << 20;
	file << 30 << 40;
	file.Close();

	file.Open("Testing.bin");
	int x = 0;
	file >> x;
	file >> x;
	file >> x;
	file >> x;
	file.Close();
}

void MainVector()
{
	Vector<int> v;
	VERUS_FOR(i, 100)
	{
		v.push_back(i * 2);
	}
	VERUS_P_FOR(i, 100)
	{
		v[i] *= 2;
	});
}

void MainRandom()
{
	Random r;
	UINT32 ui = 0;
	ui = r.Next();
	ui = r.Next();
	ui = r.Next();
	ui = r.Next();
	ui = r.Next();
	float f = 0;
	f = r.NextFloat();
	f = r.NextFloat();
	f = r.NextFloat();
	f = r.NextFloat();
	f = r.NextFloat();
}

void MainCast()
{
	try
	{
		INT64 i64 = 5ll * 1000ll * 1000ll * 1000ll;
		INT32 i32 = static_cast<int>(i64);
		i32 = Utils::Cast32(i64);
	}
	catch (D::RcRecoverable e)
	{
		std::cout << e.what() << std::endl;
	}
}

int main(VERUS_MAIN_DEFAULT_ARGS)
{
	App::Settings settings;
	settings.SetFilename("Options.json");
	settings.Save();







	App::Window wnd;
	wnd.Init();
	App::RunEventLoop();
	wnd.Done();






	MainTests();

	AlignedAllocator alloc;
	Utils::MakeEx(&alloc);

	Make_IO();
	IO::Async::I().Init();
	Make_Audio();
	VERUS_QREF_ASYS;
	asys.Init();



	Audio::SoundPwn snd;
	snd.Init(R"(C:\Home\Projects\Verus\Vertolet.ogg)");

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	IO::Async::I().Update();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));





	snd->NewSource()->Play();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000000));

	MainFile();

	MainVector();

	MainRandom();

	MainCast();

	return 0;
}
