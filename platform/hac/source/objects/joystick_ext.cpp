#include <objects/joystick_ext.hpp>
#include <utilities/npad.hpp>

#include <modules/joystickmodule_ext.hpp>
#include <modules/timer_ext.hpp>

#include <utilities/bidirectionalmap/bidirectionalmap.hpp>

using namespace love;

#define Module() (Module::GetInstance<JoystickModule<Console::HAC>>(Module::M_JOYSTICK))

// clang-format off
constexpr BidirectionalMap buttons = {
    Joystick<>::GAMEPAD_BUTTON_A,             HidNpadButton_A,
    Joystick<>::GAMEPAD_BUTTON_B,             HidNpadButton_B,
    Joystick<>::GAMEPAD_BUTTON_X,             HidNpadButton_X,
    Joystick<>::GAMEPAD_BUTTON_Y,             HidNpadButton_Y,
    Joystick<>::GAMEPAD_BUTTON_BACK,          HidNpadButton_Minus,
    Joystick<>::GAMEPAD_BUTTON_START,         HidNpadButton_Plus,
    Joystick<>::GAMEPAD_BUTTON_LEFTSHOULDER,  HidNpadButton_L,
    Joystick<>::GAMEPAD_BUTTON_RIGHTSHOULDER, HidNpadButton_R,
    Joystick<>::GAMEPAD_BUTTON_LEFTSTICK,     HidNpadButton_StickL,
    Joystick<>::GAMEPAD_BUTTON_RIGHTSTICK,    HidNpadButton_StickR,
    Joystick<>::GAMEPAD_BUTTON_DPAD_UP,       HidNpadButton_Up,
    Joystick<>::GAMEPAD_BUTTON_DPAD_DOWN,     HidNpadButton_Down,
    Joystick<>::GAMEPAD_BUTTON_DPAD_RIGHT,    HidNpadButton_Right,
    Joystick<>::GAMEPAD_BUTTON_DPAD_LEFT,     HidNpadButton_Left
};

constexpr BidirectionalMap axes = {
    Joystick<>::GAMEPAD_AXIS_LEFTX, HidNpadButton_StickLLeft  | HidNpadButton_StickLRight,
    Joystick<>::GAMEPAD_AXIS_LEFTY, HidNpadButton_StickLUp    | HidNpadButton_StickLDown,
    Joystick<>::GAMEPAD_AXIS_RIGHTX, HidNpadButton_StickRLeft | HidNpadButton_StickRRight,
    Joystick<>::GAMEPAD_AXIS_RIGHTY, HidNpadButton_StickRUp   | HidNpadButton_StickRDown,
    Joystick<>::GAMEPAD_AXIS_TRIGGERLEFT, HidNpadButton_ZL,
    Joystick<>::GAMEPAD_AXIS_TRIGGERRIGHT, HidNpadButton_ZR
};
// clang-format on

Joystick<Console::HAC>::Joystick(int id) : state {}, buttonStates {}
{
    this->instanceId = -1;
    this->id         = id;
}

Joystick<Console::HAC>::Joystick(int id, int index) : Joystick(id)
{
    this->Open(index);
}

Joystick<Console::HAC>::~Joystick()
{
    this->Close();
}

bool Joystick<Console::HAC>::Open(int index)
{
    this->Close();

    if (index == 0)
        padInitializeDefault(&this->state);
    else
        padInitialize(&this->state, (HidNpadIdType)index);

    padUpdate(&this->state);
    this->style = npad::GetStyleTag(&this->state);

    this->instanceId = index;

    if (this->style == npad::INVALID_STYLE_TAG)
        return false;

    this->playerId = (HidNpadIdType)index;

    this->guid = guid::GetGamepadGUID(this->GetGamepadType());
    this->name = guid::GetGamepadName(this->GetGamepadType());

    this->vibration = std::move(::Vibration(this->playerId, this->style));

    return this->IsConnected();
}

void Joystick<Console::HAC>::Close()
{
    this->instanceId = -1;
    this->playerId   = npad::INVALID_PLAYER_ID;
    this->state      = PadState {};

    this->vibration.SendValues(0, 0);
}

guid::GamepadType Joystick<Console::HAC>::GetGamepadType() const
{
    return *npad::styleTypes.Find(this->style);
}

void Joystick<Console::HAC>::GetDeviceInfo(int& vendor, int& product, int& version)
{
    guid::DeviceInfo info {};

    if (!guid::GetDeviceInfo(this->GetGamepadType(), info))
        return;

    vendor  = info.vendorId;
    product = info.productId;
    version = info.productVersion;
}

bool Joystick<Console::HAC>::IsConnected() const
{
    return padIsConnected(&this->state);
}

int Joystick<Console::HAC>::GetAxisCount() const
{
    if (!this->IsConnected())
        return 0;

    return guid::GetGamepadAxisCount(this->GetGamepadType());
}

int Joystick<Console::HAC>::GetButtonCount() const
{
    if (!this->IsConnected())
        return 0;

    return guid::GetGamepadButtonCount(this->GetGamepadType());
}

void Joystick<Console::HAC>::Update()
{
    padUpdate(&this->state);

    this->buttonStates.pressed  = padGetButtonsDown(&this->state);
    this->buttonStates.released = padGetButtonsUp(&this->state);
    this->buttonStates.held     = padGetButtons(&this->state);
}

bool Joystick<Console::HAC>::IsDown(JoystickInput& result)
{
    if (!this->IsConnected())
        return false;

    if (!this->buttonStates.pressed)
        return false;

    HidNpadButton button = (HidNpadButton)-1;

    const auto entries = buttons.GetEntries();

    for (size_t index = 0; index < entries.size(); index++)
    {
        button = (HidNpadButton)entries[index].second;

        if (entries[index].second == -1)
            continue;

        if (button & this->buttonStates.pressed)
        {
            this->buttonStates.pressed ^= button;
            result = { .type         = InputType::INPUT_TYPE_BUTTON,
                       .button       = entries[index].first,
                       .buttonNumber = (int)index };

            return true;
        }
    }

    return false;
}

bool Joystick<Console::HAC>::IsUp(JoystickInput& result)
{
    if (!this->IsConnected())
        return false;

    HidNpadButton button = (HidNpadButton)-1;

    if (!this->buttonStates.released)
        return false;

    const auto entries = buttons.GetEntries();

    for (size_t index = 0; index < entries.size(); index++)
    {
        button = (HidNpadButton)entries[index].second;

        if (entries[index].second == -1)
            continue;

        if (button & this->buttonStates.released)
        {
            this->buttonStates.released ^= button;
            result = { .type         = InputType::INPUT_TYPE_BUTTON,
                       .button       = entries[index].first,
                       .buttonNumber = (int)index };

            return true;
        }
    }

    return false;
}

/* helper functionality */
static constexpr float MAX_AXIS_VALUE = 32768.0f;

static float getStickPosition(PadState& state, bool horizontal, bool isLeft)
{
    auto stickState = padGetStickPos(&state, isLeft ? 0 : 1);

    float value = (horizontal) ? stickState.x : stickState.y;
    return std::clamp<float>(value / MAX_AXIS_VALUE, -1.0f, 1.0f);
}

static float getTrigger(uint64_t held, HidNpadButton trigger)
{
    if (held & trigger)
        return 1.0f;

    return 0.0f;
}

bool Joystick<Console::HAC>::IsAxisChanged(GamepadAxis axis)
{
    auto hacAxis = *axes.Find(axis);

    if (hacAxis & this->buttonStates.held)
    {
        this->buttonStates.held ^= hacAxis;
        return true;
    }

    if (hacAxis & this->buttonStates.released)
    {
        this->buttonStates.released ^= hacAxis;
        return true;
    }

    return false;
}

float Joystick<Console::HAC>::GetAxis(int index)
{
    if (!this->IsConnected() || index < 0 || index >= this->GetAxisCount())
        return 0.0f;

    // Buttons and sticks need separate code for each
    if (index < 6)
    {
        switch (this->GetGamepadType())
        {
            case guid::GAMEPAD_TYPE_JOYCON_LEFT:
            {
                if (index / 4 == 0) // No left stick, yes right stick
                    return (index / 2) ? 0 : getStickPosition(this->state, index % 2, false);
                else
                    return getTrigger(padGetButtons(&this->state),
                                      (index % 2) ? HidNpadButton_LeftSR : HidNpadButton_LeftSL);

                break;
            }
            case guid::GAMEPAD_TYPE_JOYCON_RIGHT:
            {
                if (index / 4 == 0) // No left stick, yes right stick
                    return (index / 2) ? 0 : getStickPosition(this->state, index % 2, true);
                else
                    return getTrigger(padGetButtons(&this->state),
                                      (index % 2) ? HidNpadButton_RightSR : HidNpadButton_RightSL);
                break;
            }
            default:
            {
                if (index == 0 || index == 1)
                {
                    if (index == 0)
                        return getStickPosition(this->state, true, true);

                    return getStickPosition(this->state, false, true);
                }
                else if (index == 2 || index == 3)
                {
                    if (index == 2)
                        return getStickPosition(this->state, true, false);

                    return getStickPosition(this->state, false, false);
                }
                else if (index == 4)
                {
                    const auto held = padGetButtons(&this->state);
                    return getTrigger(held, HidNpadButton_ZL);
                }
                else if (index == 5)
                {
                    const auto held = padGetButtons(&this->state);
                    return getTrigger(held, HidNpadButton_ZR);
                }
            }
        }
    }

    return 0.0f;
}

std::vector<float> Joystick<Console::HAC>::GetAxes()
{
    std::vector<float> axes {};
    int count = this->GetAxisCount();

    if (!this->IsConnected() || count <= 0)
        return axes;

    axes.reserve(count);

    for (int index = 0; index < count; index++)
        axes.push_back(this->GetAxis(index));

    return axes;
}

bool Joystick<Console::HAC>::IsDown(const std::vector<int>& buttons) const
{
    if (!this->IsConnected())
        return false;

    int count    = this->GetButtonCount();
    auto records = ::buttons.GetEntries();

    for (int button : buttons)
    {
        if (button < 0 || button >= count)
            continue;

        if (records[button].second == -1)
            continue;

        if (padGetButtons(&this->state) && records[button].second)
            return true;
    }

    return false;
}

float Joystick<Console::HAC>::GetGamepadAxis(GamepadAxis axis)
{
    if (!this->IsConnected())
        return 0.0f;

    return this->GetAxis(axis);
}

bool Joystick<Console::HAC>::IsGamepadDown(const std::vector<GamepadButton>& buttons) const
{
    for (auto button : buttons)
    {
        if (auto gamepadButton = ::buttons.Find(button);
            gamepadButton && (padGetButtons(&this->state) & (uint32_t)*gamepadButton))
            return true;
    }

    return false;
}

void Joystick<Console::HAC>::SetPlayerIndex(int index)
{
    if (!this->IsConnected())
        return;

    if (index < 0 || index > npad::MAX_JOYSTICKS)
        return;

    if (R_SUCCEEDED(hidSwapNpadAssignment(this->playerId, (HidNpadIdType)index)))
        this->playerId = (HidNpadIdType)index;
}

bool Joystick<Console::HAC>::SetVibration(float left, float right, float duration)
{
    left  = std::clamp(left, 0.0f, 1.0f);
    right = std::clamp(right, 0.0f, 1.0f);

    uint32_t length = Vibration<>::MAX;

    if (left == 0.0f && right == 0.0f)
        return this->SetVibration();

    if (!this->IsConnected())
    {
        this->SetVibration();
        return false;
    }

    if (duration >= 0.0f)
        length = std::min(duration, Vibration<>::MAX / 1000.0f);

    if (length == Vibration<>::HAPTYIC_INFINITY)
        this->vibration.SetDuration(length);
    else
        this->vibration.SetDuration(Timer<Console::HAC>::GetTime() + length);

    bool success = this->vibration.SendValues(left, right);

    if (success)
        Module()->AddVibration(&this->vibration);

    return success;
}

bool Joystick<Console::HAC>::SetVibration()
{
    return this->vibration.SendValues(0.0f, 0.0f);
}

void Joystick<Console::HAC>::GetVibration(float& left, float& right)
{
    this->vibration.GetValues(left, right);
}

bool Joystick<Console::HAC>::HasSensor(Sensor::SensorType type) const
{
    return true;
}

bool Joystick<Console::HAC>::IsSensorEnabled(Sensor::SensorType type)
{
    return this->sensors[type];
}

void Joystick<Console::HAC>::SetSensorEnabled(Sensor::SensorType type, bool enabled)
{
    if (this->sensors[type] && !enabled)
        this->sensors[type] = nullptr;
    else if (this->sensors[type] == nullptr && enabled)
    {
        SensorBase* sensor = nullptr;

        HidNpadIdType idType = this->playerId;
        if (padIsHandheld(&this->state))
            idType = HidNpadIdType_Handheld;

        if (type == Sensor::SENSOR_ACCELEROMETER)
            sensor = new Accelerometer(idType, this->style);
        else if (type == Sensor::SENSOR_GYROSCOPE)
            sensor = new Gyroscope(idType, this->style);

        sensor->SetEnabled(enabled);
        this->sensors[type] = sensor;
    }
}

std::vector<float> Joystick<Console::HAC>::GetSensorData(Sensor::SensorType type)
{
    if (!this->IsSensorEnabled(type))
    {
        auto name = Sensor::sensorTypes.ReverseFind(type);
        throw love::Exception("\"%s\" sensor is not enabled", *name);
    }

    return this->sensors[type]->GetData();
}
