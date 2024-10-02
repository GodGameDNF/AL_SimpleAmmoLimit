#include <algorithm>  // std::shuffle
#include <chrono>     // std::chrono::system_clock
#include <cmath>      // for sin, cos
#include <cstdlib>    // for rand
#include <ctime>      // for time
#include <iostream>
#include <random>  // std::default_random_engine
#include <regex>
#include <string>
#include <vector>
#include <iomanip>  // std::hex, std::uppercase
#include <unordered_set>

using namespace RE;

constexpr double DEG2RAD = 3.141592653589793 / 180.0;
PlayerCharacter* p = nullptr;
BSScript::IVirtualMachine* vm = nullptr;
TESDataHandler* DH = nullptr;
TESGlobal* gAmmoCapacityMult = nullptr;
TESGlobal* gAmmoDropDistance = nullptr;
bool bRunning = false;

std::unordered_set<BGSMod::Attachment::Mod*> saveMod;  // 중복 방지와 빠른 탐색을 위한 unordered_set 사용



struct FormOrInventoryObj
{
	TESForm* form{ nullptr };  // TESForm 포인터를 가리키는 포인터
	uint64_t second_arg{ 0 };  // unsigned 64비트 정수
};

bool AddItemVM(BSScript::IVirtualMachine* vm, uint32_t i, TESObjectREFR* target, FormOrInventoryObj obj, uint32_t count, bool b1)
{
	using func_t = decltype(&AddItemVM);
	REL::Relocation<func_t> func{ REL::ID(1212351) };
	return func(vm, i, target, obj, count, b1);
}

bool RemoveItemVM(BSScript::IVirtualMachine* vm, uint32_t i, TESObjectREFR* target, FormOrInventoryObj obj, uint32_t count, bool b1, TESObjectREFR* sender)
{
	using func_t = decltype(&RemoveItemVM);
	REL::Relocation<func_t> func{ REL::ID(492460) };
	return func(vm, i, target, obj, count, b1, sender);
}

void RemoveAllItems(BSScript::IVirtualMachine* vm, uint32_t i, TESObjectREFR* del, TESObjectREFR* send, bool bMove)
{
	using func_t = decltype(&RemoveAllItems);
	REL::Relocation<func_t> func{ REL::ID(534058) };
	return func(vm, i, del, send, bMove);
}

std::string getAmmoName(std::monostate, std::string ammoName)
{
	std::regex re("\\[.*?\\]");  // 대괄호로 둘러싸인 내용을 찾는 정규 표현식
	std::string result = std::regex_replace(ammoName, re, "");

	return result;
}

int calculateAmmoLimit(TESForm* ammo)
{
	if (!ammo) {
		return 300;  // ammo가 nullptr일 경우 300 반환
	}

	TESAmmo* tAmmo = (TESAmmo*)ammo;

	// 최소 탄약 무게를 가져오거나 기본값 설정
	TESGlobal* gTempAmmoMinWeight = (TESGlobal*)DH->LookupForm(0x00012, "AL_SimpleAmmoLimit.esp");
	float minWeight = gTempAmmoMinWeight ? gTempAmmoMinWeight->value : 0.025;
	if (minWeight < 0.01)
		minWeight = 0.01;

	float ammoWeight = tAmmo->weight;
	if (ammoWeight < minWeight)
		ammoWeight = minWeight;

	// 기본 탄약 개수를 가져오거나 기본값 설정
	TESGlobal* gbaseCapacity = (TESGlobal*)DH->LookupForm(0x00013, "AL_SimpleAmmoLimit.esp");
	int baseCapacity = gbaseCapacity ? gbaseCapacity->value : 80;

	int roundAmmoCount = 20;  // 20개 단위로 계산을 함

	// 최대 무게를 가져오거나 기본값 설정
	TESGlobal* gBaseMaxWeight = (TESGlobal*)DH->LookupForm(0x00014, "AL_SimpleAmmoLimit.esp");
	float baseMaxWeight = gBaseMaxWeight ? gBaseMaxWeight->value : 6;

	// 탄약 용량 배율을 가져오거나 기본값 설정
	TESGlobal* gAmmoLimitMult = (TESGlobal*)DH->LookupForm(0x0002, "AL_SimpleAmmoLimit.esp");
	float ammoLimitMult = gAmmoLimitMult ? gAmmoLimitMult->value : 1;

	// 탄약 용량 계산하고 20단위로 증가시킴
	int originAmmoCapacity = (baseCapacity + (baseMaxWeight / ammoWeight)) * ammoLimitMult;
	int ammoCapacity = originAmmoCapacity + (roundAmmoCount - (originAmmoCapacity % roundAmmoCount));

	return ammoCapacity;
}

int CheckDrop(std::monostate, TESForm* ammo)
{
	if (!ammo || bRunning)
		return 0;

	bRunning = true;

	int ammoCapacity = calculateAmmoLimit(ammo);

	uint32_t ammoCount = 0;
	p->GetItemCount(ammoCount, ammo, false);

	//logger::info("탄약이름 {} 탄약무게 {} 탄약수 제한 {}", tAmmo->GetFullName(), std::to_string(ammoWeight), ammoCapacity);

	if (ammoCount > ammoCapacity) {
		uint32_t dropCount = ammoCount - ammoCapacity;

		FormOrInventoryObj tempObj;
		tempObj.form = ammo;
		RemoveItemVM(vm, 0, p, tempObj, dropCount, true, nullptr);

		bRunning = false;
		return dropCount;
	}

	bRunning = false;
	return 0;
}

float getRandomAngle()
{
	// 난수 생성 엔진 및 분포 설정
	std::random_device rd;                                           // 시드를 위한 난수 생성기
	std::mt19937 gen(rd());                                          // Mersenne Twister 엔진 사용
	std::uniform_real_distribution<float> distribution(0.9f, 1.6f);  // (라디안) 균등 분포

	return distribution(gen);  // 고르게 분포된 난수 반환
}

std::vector<float> getDropPos(std::monostate)
{
	float randomOffset = getRandomAngle();                  // 무작위로 라디안 범위의 오프셋 생성
	float randomAngleRad = p->data.angle.z + randomOffset;  // 플레이어의 현재 라디안 값에 오프셋을 더함

	// 삼각함수를 사용하여 새로운 좌표 계산
	float deltaX = gAmmoDropDistance->value * std::cos(randomAngleRad);  // x 좌표의 변화량
	float deltaY = gAmmoDropDistance->value * std::sin(randomAngleRad);  // y 좌표의 변화량

	// 캐릭터의 현재 위치에서 상대적으로 계산된 좌표 반환
	return { deltaY, deltaX };
}

int getAmmoLimit(std::monostate, TESForm* ammo)
{
	if (ammo)
		return calculateAmmoLimit(ammo);

	return 1000;
}

void injectAmmo(std::monostate)
{
	TESObjectREFR* sellerChest = (TESObjectREFR*)DH->LookupForm(0x01b, "AL_SimpleAmmoLimit.esp");
	if (!sellerChest)
		return;


	//logger::info("템 갯수 f4se {}", itemArray.size());

	RemoveAllItems(vm, 0, sellerChest, nullptr, false);
	std::vector<TESAmmo*> sellAmmoArray;

	BGSInventoryList* iList = p->inventoryList;
	BSTArray<BGSInventoryItem> itemArray = iList->data;
	int itemArraySize = itemArray.size();

	// 플레이어의 소지품에서 무기를 확인하고 그 무기가 사용하는 탄약을 배열에 더함
	for (int i = 0; i < itemArraySize; ++i) {
		BGSInventoryItem tempInvenItem = itemArray[i];

		TESForm* tempForm = tempInvenItem.object;

		if (tempForm && tempForm->formType == ENUM_FORM_ID::kWEAP) {
			TESObjectWEAP* weapon = (TESObjectWEAP*)tempForm;
			if (weapon->GetPlayable(weapon->GetBaseInstanceData())) {

				BGSInventoryItem tempInvenItem = itemArray[i];
				TESAmmo* targetAmmo = weapon->weaponData.ammo;

				// 스택 데이터 가져오기
				auto stack = tempInvenItem.stackData.get();

				while (stack) {
					if (stack->extra) {
						// extra 데이터를 통해 모드 정보를 탐색
						ExtraDataList* extraData = stack->extra.get();
						if (extraData) {
							// 모드 관련 데이터를 찾아봄 (예: kExtraDataMod)
							auto modData = extraData->GetByType(kObjectInstance);
							if (modData) {
								// 모드 데이터를 BGSMod::Attachment::Mod로 캐스팅
								//BGSMod::Attachment::Mod* mod = (BGSMod::Attachment::Mod*)modData;
								BGSObjectInstanceExtra* iData = (BGSObjectInstanceExtra*)modData;

								for (const auto& tMod : saveMod) {
									if (iData->HasMod(*tMod)) {
										if (!tMod)
											continue;

										BGSMod::Attachment::Mod::Data modData;
										tMod->GetData(modData);

										for (std::uint32_t i = 0; i < modData.propertyModCount; ++i) {
											BGSMod::Property::Mod* propertyMod = &modData.propertyMods[i];

											if (propertyMod) {
												if (propertyMod->op == BGSMod::Property::OP::kSet && propertyMod->type == BGSMod::Property::TYPE::kForm) {
													TESForm* tForm = propertyMod->data.form;
													if (tForm && tForm->formType == ENUM_FORM_ID::kAMMO) {
														targetAmmo = (TESAmmo*)tForm;
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
					// 다음 스택으로 이동
					stack = stack->nextStack.get();
				}

				

				if (targetAmmo && std::find(sellAmmoArray.begin(), sellAmmoArray.end(), targetAmmo) == sellAmmoArray.end()) {
					sellAmmoArray.push_back(targetAmmo);
				}
			}
		}
	}
	

	// 배열에 저장한 탄약을 탄약제한 수량과 비교해 상인 상자에 더함
	for (TESAmmo* ammo : sellAmmoArray) {
		int ammoCapacity = calculateAmmoLimit(ammo);
		uint32_t ammoCount = 0;
		p->GetItemCount(ammoCount, ammo, false);

		if (ammoCapacity > ammoCount) {
			uint32_t sellCount = ammoCapacity - ammoCount;

			FormOrInventoryObj tempObj;
			tempObj.form = ammo;
			AddItemVM(vm, 0, sellerChest, tempObj, sellCount, false);
		}
	}

	TESGlobal* gPanalty = (TESGlobal*)DH->LookupForm(0x0020, "AL_SimpleAmmoLimit.esp");
	float decreaseCharisma = gPanalty ? gPanalty->value : -5;

	SpellItem* penaltySpell = (SpellItem*)DH->LookupForm(0x01E, "AL_SimpleAmmoLimit.esp");
	if (!penaltySpell)
		return;

	BSTArray<EffectItem*> eList = ((MagicItem*)penaltySpell)->listOfEffects;
	if (!eList.empty() && eList[0]) {
		eList[0]->data.magnitude = decreaseCharisma;
	}
}

void saveAmmoMods()
{
	auto& setAmmoModArray = DH->GetFormArray<BGSMod::Attachment::Mod>();

	int itemArraySize = setAmmoModArray.size();

	for (int i = 0; i < itemArraySize; ++i) {
		BGSMod::Attachment::Mod* tMod = setAmmoModArray[i];

		if (!tMod)
			continue;

		BGSMod::Attachment::Mod::Data modData;
		tMod->GetData(modData);

		if (modData.propertyModCount == 0) {
			continue;  // propertyModCount가 0이면 다음으로 넘어감
		}

		for (std::uint32_t i = 0; i < modData.propertyModCount; ++i) {
			BGSMod::Property::Mod* propertyMod = &modData.propertyMods[i];

			if (propertyMod) {
				if (propertyMod->op == BGSMod::Property::OP::kSet && propertyMod->type == BGSMod::Property::TYPE::kForm) {
					TESForm* tForm = propertyMod->data.form;
					if (tForm && tForm->formType == ENUM_FORM_ID::kAMMO) {
						// unordered_set으로 중복 체크 및 추가
						saveMod.insert(tMod);
					}
				}
			}
		}
	}
	logger::info("저장한 모드 숫자{}", saveMod.size());
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case F4SE::MessagingInterface::kGameLoaded:
		DH = RE::TESDataHandler::GetSingleton();
		p = PlayerCharacter::GetSingleton();
		gAmmoCapacityMult = (TESGlobal*)DH->LookupForm(0x0002, "AL_SimpleAmmoLimit.esp");
		gAmmoDropDistance = (TESGlobal*)DH->LookupForm(0x0011, "AL_SimpleAmmoLimit.esp");

		saveAmmoMods();

		break;
	}
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm)
{
	vm = a_vm;

	//REL::IDDatabase::Offset2ID o2i;
	//logger::info("0x0x80750: {}", o2i(0x80750));

	a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "injectAmmo"sv, injectAmmo);

	
	a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "CheckDrop"sv, CheckDrop);
	//a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "checkAllAmmo"sv, checkAllAmmo);
	a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "getAmmoName"sv, getAmmoName);
	a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "getDropPos"sv, getDropPos);
	a_vm->BindNativeMethod("AL_SimpleAmmoLimit"sv, "getAmmoLimit"sv, getAmmoLimit);
	
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const F4SE::PapyrusInterface* papyrus = F4SE::GetPapyrusInterface();
	if (papyrus)
		papyrus->Register(RegisterPapyrusFunctions);

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	if (message)
		message->RegisterListener(OnF4SEMessage);

	return true;
}
