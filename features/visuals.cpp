#include "visuals.h"
#include "lagcompensation.h"
#include "../core/fw_bridge.h"
#include "../core/settings.h"

bool Visuals::shouldDraw = false;

//Temporary debugging of the hit shot resolver
constexpr int HISTORY_COUNT = 2;
HistoryList<zvec3, HISTORY_COUNT> starts;
HistoryList<zvec3, HISTORY_COUNT> ends;
vec3_t start;
vec3_t end;
int best = 0;
int besti = 0;
int btTick = -1;
float lastSimtimes[MAX_PLAYERS];

vec3_t sStart, sEnd, cStart, cEnd;

vec3 traceBoxes[20];
float traceDamages[20];
int traceCount = 0;
int awallTraceCount = 0;
float awallDamage = 0;
float awallStartDamage = 0;

void Visuals::RenderAwallBoxes()
{
#ifdef PT_VISUALS
	for (int i = 0; i < awallTraceCount * 2; i++) {
		vec3 mins(-0.5f);
		vec3 maxs(0.5f);
		vec3 ang(0);
		Color faceCol(0.f, 1.f, 0.f, 0.1f);
		Color col(0.f, 1.f, 0.f, 1.f);
		debugOverlay->AddBoxOverlay2(traceBoxes[i], mins, maxs, ang, faceCol, col, 10.f);
	}
#endif
}

#ifdef PT_VISUALS
static void RenderPlayer(Players& pl, matrix4x4& w2s, vec2 screen, Color col);

static void DrawText(const char* text, Color color, int x, int y)
{
	wchar_t buf[128];

	size_t len = mbstowcs(buf, text, 128);

	if (len > 0) {
		surface->DrawSetTextColor(color);
		surface->DrawSetTextPos(x, y);
		surface->DrawPrintText(buf, len);
	}

}

static bool CheckHitboxes(Players& p1, int p1ID, Players& p2, int p2ID)
{
	vec3_t origDelta = p1.origin[p1ID] - p2.origin[p2ID];

	if (origDelta.LengthSqr<3>() < 100)
		return false;

	float maxDist = 0;

	for (int i = 0; i < MAX_HITBOXES; i++)
		maxDist = fmaxf(maxDist, (p1.hitboxes[p1ID].wm[i].Vector3Transform(vec3_t(0)) - p2.hitboxes[p2ID].wm[i].Vector3Transform(vec3_t(0)) + origDelta).LengthSqr<3>());

	return maxDist < 50;
}

[[maybe_unused]] static void Draw3DLine(vec3_t start, vec3_t end, Color col, const matrix4x4& w2s, vec2 screen)
{
	bool flag1, flag2;

	vec3_t startPos = w2s.WorldToScreen(start, screen, flag1);
	vec3_t endPos = w2s.WorldToScreen(end, screen, flag2);

	if (!flag1 && !flag2)
		return;

	surface->DrawSetColor(col);
	surface->DrawLine(startPos[0], startPos[1], endPos[0], endPos[1]);
}


void Visuals::Draw()
{
	if (!engine->IsInGame()) {
		shouldDraw = false;
		memset(lastSimtimes, 0, sizeof(lastSimtimes));
		return;
	}

	if (!shouldDraw)
		return;

	static matrix4x4& w2s = (matrix4x4&)engine->WorldToScreenMatrix();

	vec2 screen;
	int w, h;
	engine->GetScreenSize(w, h);
	screen[0] = w;
	screen[1] = h;

	for (size_t i = 0; i < 1 && i < FwBridge::playerTrack.Count(); i+=1)
		RenderPlayer(FwBridge::playerTrack.GetLastItem(i), w2s, screen, Color(1.f, 0.f, 0.f, 1.f));
	if (LagCompensation::futureTrack) {
		for (size_t i = 0; i < 0 && i < 1 * LagCompensation::futureTrack->Count(); i+=1)
			RenderPlayer(LagCompensation::futureTrack->GetLastItem(i), w2s, screen, Color(0.f, 0.f, 1.f, 1.f));
	}

	//Draw3DLine(cStart, cEnd, Color(1.f, 0.f, 0.f, 1.f), w2s, screen);
	//Draw3DLine(sStart, sEnd, Color(0.f, 1.f, 0.f, 1.f), w2s, screen);


	Players& curP = FwBridge::playerTrack[0];

	bool rendered[MAX_PLAYERS];
	memset(rendered, 0, MAX_PLAYERS);

	for (size_t i = 1; i < FwBridge::playerTrack.Count(); i++) {
		Players& p = FwBridge::playerTrack[i];
		for (int o = 0; o < curP.count; o++) {
			int pID = curP.Resort(p, o);
			if (pID < p.count) {
				//TODO: Compare hitboxes and draw the same ones
				if (0 && !rendered[o] && lastSimtimes[curP.unsortIDs[o]] < p.time[pID] && CheckHitboxes(p, pID, curP, o)) {
					lastSimtimes[curP.unsortIDs[o]] = p.time[pID];
					RenderPlayerCapsules(p, Color(1.f, 0.f, 0.f, 1.f), pID);
					rendered[o] = true;
				}
			}
		}
	}

	for (int i = 0; i < 0 && i < HISTORY_COUNT; i++) {
		bool flags[16];
		zvec3 screenStartPos = w2s.WorldToScreen(starts.GetLastItem(i), screen, flags);
		bool flags2[16];
		zvec3 screenEndPos = w2s.WorldToScreen(ends.GetLastItem(i), screen, flags2);

		for (size_t u = 0; u < 16; u++) {
			if (!flags[u] || !flags2[u])
				continue;
			vec3 screenStart = (vec3)screenStartPos.acc[u];
			vec3 screenEnd = (vec3)screenEndPos.acc[u];
			if (best == (int)u && besti == i)
				surface->DrawSetColor(Color(1.f, 0.f, 1.f, 1.f));
			else
				surface->DrawSetColor(Color(0.f, 0.f, 1.f, 1.f));
			surface->DrawLine(screenStart[0], screenStart[1], screenEnd[0], screenEnd[1]);
		}
	}


	if (false) {
		bool flag1, flag2;

		vec3_t startPos = w2s.WorldToScreen(start, screen, flag1);
		vec3_t endPos = w2s.WorldToScreen(end, screen, flag2);

		if (!flag1 || !flag2)
			return;

		surface->DrawSetColor(Color(0.f, 1.f, 0.f, 1.f));
		surface->DrawLine(startPos[0], startPos[1], endPos[0], endPos[1]);
	}

	int traceCount = FwBridge::traceCountAvg;
	int traceTime = FwBridge::traceTimeAvg;

	char buf[128];

	snprintf(buf, 128, "%d traces @ %.2f milliseconds", traceCount, ((float)traceTime) / 1000);
	DrawText(buf, Color(0.3f, 1.f, 0.f, 1.f), 12, 16);

	snprintf(buf, 128, "Autowall dmg: %f\n", awallDamage);
	DrawText(buf, Color(0.3f, 1.f, 0.f, 1.f), 12, 16 + 17);

	float cdmg = awallStartDamage;

	for (int i = 0; i < awallTraceCount; i++) {
		cdmg -= traceDamages[i];
		snprintf(buf, 128, "-%f -> %f(%.1f %.1f %.1f)", traceDamages[i], cdmg, traceBoxes[i * 2 + 1][0], traceBoxes[i * 2 + 1][1], traceBoxes[i * 2 + 1][2]);
		DrawText(buf, Color(0.3f, 1.f, 0.f, 1.f), 12, 16 + 17 * (i + 2));

	}
}

static void RenderPlayer(Players& pl, matrix4x4& w2s, vec2 screen, Color col)
{

	int count = pl.count;

	for (int i = 0; i < count; i++) {

		if (pl.time[i] < globalVars->curtime - 0.2f)
			continue;

		if (Settings::debugVisuals > 1) {
			for (auto u : Settings::aimbotHitboxes) {

				if (FwBridge::hitboxIDs[u.hitbox] < 0 || !u.mask)
					continue;

				int o = FwBridge::hitboxIDs[u.hitbox];

				mvec3 mpVec = pl.hitboxes[i].mpOffset[o];
				mvec3 ptVec = pl.hitboxes[i].mpDir[o] * pl.hitboxes[i].radius[o] * u.pointScale;
				mpVec += ptVec;
				mpVec = pl.hitboxes[i].wm[o].VecSoaTransform(mpVec);

				bool flags[mpVec.Yt];
				mvec3 screenPos = w2s.WorldToScreen(mpVec, screen, flags);

				for (size_t u = 0; u < 1 + 0 *MULTIPOINT_COUNT; u++) {
					if (!flags[u])
						continue;
					vec3 screen = (vec3)screenPos.acc[u];
					surface->DrawSetColor(col);
					surface->DrawFilledRect(screen[0]-2, screen[1]-2, screen[0]+2, screen[1]+2);
				}
			}
		}

		vec3_t start = pl.origin[i];
		vec3_t offset(0, 0, 30);
		vec3_t end = start + offset;
		bool flags = false, flags2 = false;
		vec3_t screenPos = w2s.WorldToScreen(start, screen, flags);
		vec3_t screenPosEnd = w2s.WorldToScreen(end, screen, flags2);

		if (flags && flags2) {
			surface->DrawSetColor((pl.flags[i] & Flags::ONGROUND) ? (~pl.flags[i] & Flags::FRIENDLY ? Color(1.f, 0.3f, 0.f, 1.f) : Color(0.3f, 0.6f, 0.f, 1.f)) : Color(0.f, 0.3f, 1.f));
			surface->DrawFilledRect(screenPosEnd[0]-2, screenPosEnd[1]-2, screenPos[0]+2, screenPos[1]+2);
		}
	}

}

void Visuals::RenderPlayerCapsules(Players& pl, Color col, int id)
{
	int count = pl.count;

	Color faceCol = col;
	faceCol[3] /= 10;

	for (int i = 0; i < count; i++) {
		if (id < 0 || id == i)
			for (int o = 0; o < MAX_HITBOXES; o++) {
				if (pl.hitboxes[i].radius[o] >= 0) {
					vec3 mins = pl.hitboxes[i].wm[o].Vector3Transform(pl.hitboxes[i].start[o]);
					vec3 maxs = pl.hitboxes[i].wm[o].Vector3Transform(pl.hitboxes[i].end[o]);
					debugOverlay->AddCapsuleOverlay(mins, maxs, pl.hitboxes[i].radius[o], col, 5.f);
				} else {
					vec3 origin = pl.hitboxes[i].wm[o].Vector3Transform(vec3(0, 0, 0));
					vec3 ang = pl.hitboxes[i].wm[o].GetAngles(true);
					vec3 mins = pl.hitboxes[i].start[o];
					vec3 maxs = pl.hitboxes[i].end[o];
					debugOverlay->AddBoxOverlay2(origin, mins, maxs, ang, faceCol, col, 5.f);
				}
			}
	}
}

#endif

void Visuals::PassColliders(vec3soa<float, 16> start, vec3soa<float, 16> end)
{
	starts.Push(start);
	ends.Push(end);
}


void Visuals::PassStart(vec3_t start, vec3_t end)
{
	::start = start;
	::end = end;
}

void Visuals::PassBest(int o, int i)
{
	besti = i;
	best = o;
}

void Visuals::SetShotVectors(vec3_t serverStart, vec3_t serverEnd, vec3_t idealStart, vec3_t idealEnd)
{
	sStart = serverStart;
	sEnd = serverEnd;
	cStart = idealStart;
	cEnd = idealEnd;
}

void Visuals::SetAwallBoxes(vec3_t* boxes, float* damages, int count, float outDamage, float startDamage)
{
#ifdef DEBUG
	for (int i = 0; i < count * 2; i++)
		traceBoxes[i] = boxes[i];

	for (int i = 0; i < count; i++)
			traceDamages[i] = damages[i];

	awallStartDamage = startDamage;

	awallTraceCount = count;
#else
	awallTraceCount = 0;
#endif
	awallDamage = outDamage;
}
