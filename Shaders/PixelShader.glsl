#version 460 core

layout(location = 0) out vec4 Fragcolor;

uniform ivec2 resolution;
uniform int frame;
uniform int samples;
uniform int prevSamples;
uniform float FPS;
uniform vec2 cameraAngle;
uniform vec3 cameraPos;
uniform float persistance;
uniform int ISO;
uniform float cameraSize;
uniform float apertureSize;
uniform float apertureDist;
uniform vec4 lensData;
uniform int samplesPerFrame;
uniform int pathLength;
//uniform float CIEXYZ2006[1323];
uniform int numObjects[];
uniform float objects[512];
uniform float materials[512];
uniform sampler2D screenTexture;

#define MAXDIST 1e5
#define PI 3.141592653589792623810034526344

struct Ray {
	vec3 origin;
	vec3 dir;
};

struct sphere {
	vec3 pos;
	float radius;
	int materialID;
};

struct plane {
	vec3 pos;
	int materialID;
};

struct box {
	vec3 pos;
	vec3 rotation;
	vec3 size;
	int materialID;
};

struct sphereSlice {
	vec3 pos;
	float radius;
	float sliceSize;
	bool is1stSlice;
	vec3 rotation;
	int materialID;
};

struct lens {
	vec3 pos;
	vec3 rotation;
	float radius;
	float focalLength;
	float thickness;
	bool isConverging;
	int materialID;
};

struct material {
	vec3 reflection;
	vec2 emission;
};


// Table
// http://www.cvrl.org/ciexyzpr.htm
float CIEXYZ2006[1323] = float[](
    0.00295242f, 	0.0004076779f, 	0.01318752f,
    0.003577275f, 	0.0004977769f, 	0.01597879f,
    0.004332146f, 	0.0006064754f, 	0.01935758f,
    0.005241609f, 	0.000737004f, 	0.02343758f,
    0.006333902f, 	0.0008929388f, 	0.02835021f,
    0.007641137f, 	0.001078166f, 	0.03424588f,
    0.009199401f, 	0.001296816f, 	0.04129467f,
    0.01104869f, 	0.001553159f, 	0.04968641f,
    0.01323262f, 	0.001851463f, 	0.05962964f,
    0.01579791f, 	0.002195795f, 	0.07134926f,
    0.01879338f, 	0.002589775f, 	0.08508254f,
    0.02226949f, 	0.003036799f, 	0.1010753f,
    0.02627978f, 	0.003541926f, 	0.1195838f,
    0.03087862f, 	0.004111422f, 	0.1408647f,
    0.0361189f, 	0.004752618f, 	0.1651644f,
    0.04204986f, 	0.005474207f, 	0.1927065f,
    0.04871256f, 	0.006285034f, 	0.2236782f,
    0.05612868f, 	0.007188068f, 	0.2582109f,
    0.06429866f, 	0.008181786f, 	0.2963632f,
    0.07319818f, 	0.009260417f, 	0.3381018f,
    0.08277331f, 	0.01041303f, 	0.3832822f,
    0.09295327f, 	0.01162642f, 	0.4316884f,
    0.1037137f, 	0.01289884f, 	0.483244f,
    0.115052f, 	0.01423442f, 	0.5379345f,
    0.1269771f, 	0.0156408f, 	0.595774f,
    0.1395127f, 	0.01712968f, 	0.6568187f,
    0.1526661f, 	0.01871265f, 	0.7210459f,
    0.1663054f, 	0.02038394f, 	0.7878635f,
    0.1802197f, 	0.02212935f, 	0.8563391f,
    0.1941448f, 	0.02392985f, 	0.9253017f,
    0.2077647f, 	0.02576133f, 	0.9933444f,
    0.2207911f, 	0.02760156f, 	1.059178f,
    0.2332355f, 	0.02945513f, 	1.122832f,
    0.2452462f, 	0.03133884f, 	1.184947f,
    0.2570397f, 	0.03327575f, 	1.246476f,
    0.2688989f, 	0.03529554f, 	1.308674f,
    0.2810677f, 	0.03742705f, 	1.372628f,
    0.2933967f, 	0.03967137f, 	1.437661f,
    0.3055933f, 	0.04201998f, 	1.502449f,
    0.3173165f, 	0.04446166f, 	1.565456f,
    0.3281798f, 	0.04698226f, 	1.62494f,
    0.3378678f, 	0.04956742f, 	1.679488f,
    0.3465097f, 	0.05221219f, 	1.729668f,
    0.3543953f, 	0.05491387f, 	1.776755f,
    0.3618655f, 	0.05766919f, 	1.822228f,
    0.3693084f, 	0.06047429f, 	1.867751f,
    0.3770107f, 	0.06332195f, 	1.914504f,
    0.384685f, 	0.06619271f, 	1.961055f,
    0.3918591f, 	0.06906185f, 	2.005136f,
    0.3980192f, 	0.0719019f, 	2.044296f,
    0.4026189f, 	0.07468288f, 	2.075946f,
    0.4052637f, 	0.07738452f, 	2.098231f,
    0.4062482f, 	0.08003601f, 	2.112591f,
    0.406066f, 	0.08268524f, 	2.121427f,
    0.4052283f, 	0.08538745f, 	2.127239f,
    0.4042529f, 	0.08820537f, 	2.132574f,
    0.4034808f, 	0.09118925f, 	2.139093f,
    0.4025362f, 	0.09431041f, 	2.144815f,
    0.4008675f, 	0.09751346f, 	2.146832f,
    0.3979327f, 	0.1007349f, 	2.14225f,
    0.3932139f, 	0.103903f, 	2.128264f,
    0.3864108f, 	0.1069639f, 	2.103205f,
    0.3779513f, 	0.1099676f, 	2.069388f,
    0.3684176f, 	0.1129992f, 	2.03003f,
    0.3583473f, 	0.1161541f, 	1.988178f,
    0.3482214f, 	0.1195389f, 	1.946651f,
    0.338383f, 	0.1232503f, 	1.907521f,
    0.3288309f, 	0.1273047f, 	1.870689f,
    0.3194977f, 	0.1316964f, 	1.835578f,
    0.3103345f, 	0.1364178f, 	1.801657f,
    0.3013112f, 	0.1414586f, 	1.76844f,
    0.2923754f, 	0.1468003f, 	1.735338f,
    0.2833273f, 	0.1524002f, 	1.701254f,
    0.2739463f, 	0.1582021f, 	1.665053f,
    0.2640352f, 	0.16414f, 	1.625712f,
    0.2534221f, 	0.1701373f, 	1.582342f,
    0.2420135f, 	0.1761233f, 	1.534439f,
    0.2299346f, 	0.1820896f, 	1.482544f,
    0.2173617f, 	0.1880463f, 	1.427438f,
    0.2044672f, 	0.1940065f, 	1.369876f,
    0.1914176f, 	0.1999859f, 	1.310576f,
    0.1783672f, 	0.2060054f, 	1.250226f,
    0.1654407f, 	0.2120981f, 	1.189511f,
    0.1527391f, 	0.2183041f, 	1.12905f,
    0.1403439f, 	0.2246686f, 	1.069379f,
    0.1283167f, 	0.2312426f, 	1.010952f,
    0.1167124f, 	0.2380741f, 	0.9541809f,
    0.1056121f, 	0.2451798f, 	0.8995253f,
    0.09508569f, 	0.2525682f, 	0.847372f,
    0.08518206f, 	0.2602479f, 	0.7980093f,
    0.0759312f, 	0.2682271f, 	0.7516389f,
    0.06733159f, 	0.2765005f, 	0.7082645f,
    0.05932018f, 	0.2850035f, 	0.6673867f,
    0.05184106f, 	0.2936475f, 	0.6284798f,
    0.04486119f, 	0.3023319f, 	0.5911174f,
    0.0383677f, 	0.3109438f, 	0.5549619f,
    0.03237296f, 	0.3194105f, 	0.5198843f,
    0.02692095f, 	0.3278683f, 	0.4862772f,
    0.0220407f, 	0.3365263f, 	0.4545497f,
    0.01773951f, 	0.3456176f, 	0.4249955f,
    0.01400745f, 	0.3554018f, 	0.3978114f,
    0.01082291f, 	0.3660893f, 	0.3730218f,
    0.008168996f, 	0.3775857f, 	0.3502618f,
    0.006044623f, 	0.389696f, 	0.3291407f,
    0.004462638f, 	0.4021947f, 	0.3093356f,
    0.00344681f, 	0.4148227f, 	0.2905816f,
    0.003009513f, 	0.4273539f, 	0.2726773f,
    0.003090744f, 	0.4398206f, 	0.2555143f,
    0.003611221f, 	0.452336f, 	0.2390188f,
    0.004491435f, 	0.4650298f, 	0.2231335f,
    0.005652072f, 	0.4780482f, 	0.2078158f,
    0.007035322f, 	0.4915173f, 	0.1930407f,
    0.008669631f, 	0.5054224f, 	0.1788089f,
    0.01060755f, 	0.5197057f, 	0.1651287f,
    0.01290468f, 	0.5343012f, 	0.1520103f,
    0.01561956f, 	0.5491344f, 	0.1394643f,
    0.0188164f, 	0.5641302f, 	0.1275353f,
    0.02256923f, 	0.5792416f, 	0.1163771f,
    0.02694456f, 	0.5944264f, 	0.1061161f,
    0.0319991f, 	0.6096388f, 	0.09682266f,
    0.03778185f, 	0.6248296f, 	0.08852389f,
    0.04430635f, 	0.6399656f, 	0.08118263f,
    0.05146516f, 	0.6550943f, 	0.07463132f,
    0.05912224f, 	0.6702903f, 	0.06870644f,
    0.0671422f, 	0.6856375f, 	0.06327834f,
    0.07538941f, 	0.7012292f, 	0.05824484f,
    0.08376697f, 	0.7171103f, 	0.05353812f,
    0.09233581f, 	0.7330917f, 	0.04914863f,
    0.101194f, 	0.7489041f, 	0.04507511f,
    0.1104362f, 	0.764253f, 	0.04131175f,
    0.1201511f, 	0.7788199f, 	0.03784916f,
    0.130396f, 	0.792341f, 	0.03467234f,
    0.141131f, 	0.804851f, 	0.03175471f,
    0.1522944f, 	0.8164747f, 	0.02907029f,
    0.1638288f, 	0.827352f, 	0.02659651f,
    0.1756832f, 	0.8376358f, 	0.02431375f,
    0.1878114f, 	0.8474653f, 	0.02220677f,
    0.2001621f, 	0.8568868f, 	0.02026852f,
    0.2126822f, 	0.8659242f, 	0.01849246f,
    0.2253199f, 	0.8746041f, 	0.01687084f,
    0.2380254f, 	0.8829552f, 	0.01539505f,
    0.2507787f, 	0.8910274f, 	0.0140545f,
    0.2636778f, 	0.8989495f, 	0.01283354f,
    0.2768607f, 	0.9068753f, 	0.01171754f,
    0.2904792f, 	0.9149652f, 	0.01069415f,
    0.3046991f, 	0.9233858f, 	0.009753f,
    0.3196485f, 	0.9322325f, 	0.008886096f,
    0.3352447f, 	0.9412862f, 	0.008089323f,
    0.351329f, 	0.9502378f, 	0.007359131f,
    0.3677148f, 	0.9587647f, 	0.006691736f,
    0.3841856f, 	0.9665325f, 	0.006083223f,
    0.4005312f, 	0.9732504f, 	0.005529423f,
    0.4166669f, 	0.9788415f, 	0.005025504f,
    0.432542f, 	0.9832867f, 	0.004566879f,
    0.4481063f, 	0.986572f, 	0.004149405f,
    0.4633109f, 	0.9886887f, 	0.003769336f,
    0.478144f, 	0.9897056f, 	0.003423302f,
    0.4927483f, 	0.9899849f, 	0.003108313f,
    0.5073315f, 	0.9899624f, 	0.00282165f,
    0.5221315f, 	0.9900731f, 	0.00256083f,
    0.537417f, 	0.99075f, 	0.002323578f,
    0.5534217f, 	0.9922826f, 	0.002107847f,
    0.5701242f, 	0.9943837f, 	0.001911867f,
    0.5874093f, 	0.9966221f, 	0.001734006f,
    0.6051269f, 	0.9985649f, 	0.001572736f,
    0.6230892f, 	0.9997775f, 	0.001426627f,
    0.6410999f, 	0.999944f, 	0.001294325f,
    0.6590659f, 	0.99922f, 	0.001174475f,
    0.6769436f, 	0.9978793f, 	0.001065842f,
    0.6947143f, 	0.9961934f, 	0.0009673215f,
    0.7123849f, 	0.9944304f, 	0.0008779264f,
    0.7299978f, 	0.9927831f, 	0.0007967847f,
    0.7476478f, 	0.9911578f, 	0.0007231502f,
    0.765425f, 	0.9893925f, 	0.0006563501f,
    0.7834009f, 	0.9873288f, 	0.0005957678f,
    0.8016277f, 	0.9848127f, 	0.0005408385f,
    0.8201041f, 	0.9817253f, 	0.0004910441f,
    0.8386843f, 	0.9780714f, 	0.0004459046f,
    0.8571936f, 	0.973886f, 	0.0004049826f,
    0.8754652f, 	0.9692028f, 	0.0003678818f,
    0.8933408f, 	0.9640545f, 	0.0003342429f,
    0.9106772f, 	0.9584409f, 	0.0003037407f,
    0.9273554f, 	0.9522379f, 	0.0002760809f,
    0.9432502f, 	0.9452968f, 	0.000250997f,
    0.9582244f, 	0.9374773f, 	0.0002282474f,
    0.9721304f, 	0.9286495f, 	0.0002076129f,
    0.9849237f, 	0.9187953f, 	0.0001888948f,
    0.9970067f, 	0.9083014f, 	0.0001719127f,
    1.008907f, 	0.8976352f, 	0.000156503f,
    1.021163f, 	0.8872401f, 	0.0001425177f,
    1.034327f, 	0.877536f, 	0.000129823f,
    1.048753f, 	0.868792f, 	0.0001182974f,
    1.063937f, 	0.8607474f, 	0.000107831f,
    1.079166f, 	0.8530233f, 	0.00009832455f,
    1.093723f, 	0.8452535f, 	0.00008968787f,
    1.106886f, 	0.8370838f, 	0.00008183954f,
    1.118106f, 	0.8282409f, 	0.00007470582f,
    1.127493f, 	0.818732f, 	0.00006821991f,
    1.135317f, 	0.8086352f, 	0.00006232132f,
    1.141838f, 	0.7980296f, 	0.00005695534f,
    1.147304f, 	0.786995f, 	0.00005207245f,
    1.151897f, 	0.775604f, 	0.00004762781f,
    1.155582f, 	0.7638996f, 	0.00004358082f,
    1.158284f, 	0.7519157f, 	0.00003989468f,
    1.159934f, 	0.7396832f, 	0.00003653612f,
    1.160477f, 	0.7272309f, 	0.00003347499f,
    1.15989f, 	0.7145878f, 	0.000030684f,
    1.158259f, 	0.7017926f, 	0.00002813839f,
    1.155692f, 	0.6888866f, 	0.00002581574f,
    1.152293f, 	0.6759103f, 	0.00002369574f,
    1.148163f, 	0.6629035f, 	0.00002175998f,
    1.143345f, 	0.6498911f, 	0.00001999179f,
    1.137685f, 	0.636841f, 	0.00001837603f,
    1.130993f, 	0.6237092f, 	0.00001689896f,
    1.123097f, 	0.6104541f, 	0.00001554815f,
    1.113846f, 	0.5970375f, 	0.00001431231f,
    1.103152f, 	0.5834395f, 	0.00001318119f,
    1.091121f, 	0.5697044f, 	0.00001214548f,
    1.077902f, 	0.5558892f, 	0.00001119673f,
    1.063644f, 	0.5420475f, 	0.00001032727f,
    1.048485f, 	0.5282296f, 	0.00000953013f,
    1.032546f, 	0.5144746f, 	0.000008798979f,
    1.01587f, 	0.5007881f, 	0.000008128065f,
    0.9984859f, 	0.4871687f, 	0.00000751216f,
    0.9804227f, 	0.473616f, 	0.000006946506f,
    0.9617111f, 	0.4601308f, 	0.000006426776f,
    0.9424119f, 	0.446726f, 	0.0f,
    0.9227049f, 	0.4334589f, 	0.0f,
    0.9027804f, 	0.4203919f, 	0.0f,
    0.8828123f, 	0.407581f, 	0.0f,
    0.8629581f, 	0.3950755f, 	0.0f,
    0.8432731f, 	0.3828894f, 	0.0f,
    0.8234742f, 	0.370919f, 	0.0f,
    0.8032342f, 	0.3590447f, 	0.0f,
    0.7822715f, 	0.3471615f, 	0.0f,
    0.7603498f, 	0.3351794f, 	0.0f,
    0.7373739f, 	0.3230562f, 	0.0f,
    0.713647f, 	0.3108859f, 	0.0f,
    0.6895336f, 	0.298784f, 	0.0f,
    0.6653567f, 	0.2868527f, 	0.0f,
    0.6413984f, 	0.2751807f, 	0.0f,
    0.6178723f, 	0.2638343f, 	0.0f,
    0.5948484f, 	0.252833f, 	0.0f,
    0.57236f, 	0.2421835f, 	0.0f,
    0.5504353f, 	0.2318904f, 	0.0f,
    0.5290979f, 	0.2219564f, 	0.0f,
    0.5083728f, 	0.2123826f, 	0.0f,
    0.4883006f, 	0.2031698f, 	0.0f,
    0.4689171f, 	0.1943179f, 	0.0f,
    0.4502486f, 	0.185825f, 	0.0f,
    0.4323126f, 	0.1776882f, 	0.0f,
    0.415079f, 	0.1698926f, 	0.0f,
    0.3983657f, 	0.1623822f, 	0.0f,
    0.3819846f, 	0.1550986f, 	0.0f,
    0.3657821f, 	0.1479918f, 	0.0f,
    0.3496358f, 	0.1410203f, 	0.0f,
    0.3334937f, 	0.1341614f, 	0.0f,
    0.3174776f, 	0.1274401f, 	0.0f,
    0.3017298f, 	0.1208887f, 	0.0f,
    0.2863684f, 	0.1145345f, 	0.0f,
    0.27149f, 	0.1083996f, 	0.0f,
    0.2571632f, 	0.1025007f, 	0.0f,
    0.2434102f, 	0.09684588f, 	0.0f,
    0.2302389f, 	0.09143944f, 	0.0f,
    0.2176527f, 	0.08628318f, 	0.0f,
    0.2056507f, 	0.08137687f, 	0.0f,
    0.1942251f, 	0.07671708f, 	0.0f,
    0.183353f, 	0.07229404f, 	0.0f,
    0.1730097f, 	0.06809696f, 	0.0f,
    0.1631716f, 	0.06411549f, 	0.0f,
    0.1538163f, 	0.06033976f, 	0.0f,
    0.144923f, 	0.05676054f, 	0.0f,
    0.1364729f, 	0.05336992f, 	0.0f,
    0.1284483f, 	0.05016027f, 	0.0f,
    0.120832f, 	0.04712405f, 	0.0f,
    0.1136072f, 	0.04425383f, 	0.0f,
    0.1067579f, 	0.04154205f, 	0.0f,
    0.1002685f, 	0.03898042f, 	0.0f,
    0.09412394f, 	0.03656091f, 	0.0f,
    0.08830929f, 	0.03427597f, 	0.0f,
    0.0828101f, 	0.03211852f, 	0.0f,
    0.07761208f, 	0.03008192f, 	0.0f,
    0.07270064f, 	0.02816001f, 	0.0f,
    0.06806167f, 	0.02634698f, 	0.0f,
    0.06368176f, 	0.02463731f, 	0.0f,
    0.05954815f, 	0.02302574f, 	0.0f,
    0.05564917f, 	0.02150743f, 	0.0f,
    0.05197543f, 	0.02007838f, 	0.0f,
    0.04851788f, 	0.01873474f, 	0.0f,
    0.04526737f, 	0.01747269f, 	0.0f,
    0.04221473f, 	0.01628841f, 	0.0f,
    0.03934954f, 	0.01517767f, 	0.0f,
    0.0366573f, 	0.01413473f, 	0.0f,
    0.03412407f, 	0.01315408f, 	0.0f,
    0.03173768f, 	0.01223092f, 	0.0f,
    0.02948752f, 	0.01136106f, 	0.0f,
    0.02736717f, 	0.0105419f, 	0.0f,
    0.02538113f, 	0.00977505f, 	0.0f,
    0.02353356f, 	0.009061962f, 	0.0f,
    0.02182558f, 	0.008402962f, 	0.0f,
    0.0202559f, 	0.007797457f, 	0.0f,
    0.01881892f, 	0.00724323f, 	0.0f,
    0.0174993f, 	0.006734381f, 	0.0f,
    0.01628167f, 	0.006265001f, 	0.0f,
    0.01515301f, 	0.005830085f, 	0.0f,
    0.0141023f, 	0.005425391f, 	0.0f,
    0.01312106f, 	0.005047634f, 	0.0f,
    0.01220509f, 	0.00469514f, 	0.0f,
    0.01135114f, 	0.004366592f, 	0.0f,
    0.01055593f, 	0.004060685f, 	0.0f,
    0.009816228f, 	0.00377614f, 	0.0f,
    0.009128517f, 	0.003511578f, 	0.0f,
    0.008488116f, 	0.003265211f, 	0.0f,
    0.007890589f, 	0.003035344f, 	0.0f,
    0.007332061f, 	0.002820496f, 	0.0f,
    0.006809147f, 	0.002619372f, 	0.0f,
    0.006319204f, 	0.00243096f, 	0.0f,
    0.005861036f, 	0.002254796f, 	0.0f,
    0.005433624f, 	0.002090489f, 	0.0f,
    0.005035802f, 	0.001937586f, 	0.0f,
    0.004666298f, 	0.001795595f, 	0.0f,
    0.00432375f, 	0.001663989f, 	0.0f,
    0.004006709f, 	0.001542195f, 	0.0f,
    0.003713708f, 	0.001429639f, 	0.0f,
    0.003443294f, 	0.001325752f, 	0.0f,
    0.003194041f, 	0.00122998f, 	0.0f,
    0.002964424f, 	0.001141734f, 	0.0f,
    0.002752492f, 	0.001060269f, 	0.0f,
    0.002556406f, 	0.0009848854f, 	0.0f,
    0.002374564f, 	0.0009149703f, 	0.0f,
    0.002205568f, 	0.0008499903f, 	0.0f,
    0.002048294f, 	0.0007895158f, 	0.0f,
    0.001902113f, 	0.0007333038f, 	0.0f,
    0.001766485f, 	0.0006811458f, 	0.0f,
    0.001640857f, 	0.0006328287f, 	0.0f,
    0.001524672f, 	0.0005881375f, 	0.0f,
    0.001417322f, 	0.0005468389f, 	0.0f,
    0.001318031f, 	0.0005086349f, 	0.0f,
    0.001226059f, 	0.0004732403f, 	0.0f,
    0.001140743f, 	0.0004404016f, 	0.0f,
    0.001061495f, 	0.0004098928f, 	0.0f,
    0.0009877949f, 	0.0003815137f, 	0.0f,
    0.0009191847f, 	0.0003550902f, 	0.0f,
    0.0008552568f, 	0.0003304668f, 	0.0f,
    0.0007956433f, 	0.000307503f, 	0.0f,
    0.000740012f, 	0.0002860718f, 	0.0f,
    0.000688098f, 	0.0002660718f, 	0.0f,
    0.0006397864f, 	0.0002474586f, 	0.0f,
    0.0005949726f, 	0.0002301919f, 	0.0f,
    0.0005535291f, 	0.0002142225f, 	0.0f,
    0.0005153113f, 	0.0001994949f, 	0.0f,
    0.0004801234f, 	0.0001859336f, 	0.0f,
    0.0004476245f, 	0.0001734067f, 	0.0f,
    0.0004174846f, 	0.0001617865f, 	0.0f,
    0.0003894221f, 	0.0001509641f, 	0.0f,
    0.0003631969f, 	0.0001408466f, 	0.0f,
    0.0003386279f, 	0.0001313642f, 	0.0f,
    0.0003156452f, 	0.0001224905f, 	0.0f,
    0.0002941966f, 	0.000114206f, 	0.0f,
    0.0002742235f, 	0.0001064886f, 	0.0f,
    0.0002556624f, 	0.00009931439f, 	0.0f,
    0.000238439f, 	0.00009265512f, 	0.0f,
    0.0002224525f, 	0.00008647225f, 	0.0f,
    0.0002076036f, 	0.0000807278f, 	0.0f,
    0.0001938018f, 	0.00007538716f, 	0.0f,
    0.0001809649f, 	0.00007041878f, 	0.0f,
    0.0001690167f, 	0.00006579338f, 	0.0f,
    0.0001578839f, 	0.0000614825f, 	0.0f,
    0.0001474993f, 	0.00005746008f, 	0.0f,
    0.0001378026f, 	0.00005370272f, 	0.0f,
    0.0001287394f, 	0.00005018934f, 	0.0f,
    0.0001202644f, 	0.00004690245f, 	0.0f,
    0.0001123502f, 	0.00004383167f, 	0.0f,
    0.0001049725f, 	0.0000409678f, 	0.0f,
    0.00009810596f, 	0.00003830123f, 	0.0f,
    0.00009172477f, 	0.00003582218f, 	0.0f,
    0.00008579861f, 	0.00003351903f, 	0.0f,
    0.00008028174f, 	0.00003137419f, 	0.0f,
    0.00007513013f, 	0.00002937068f, 	0.0f,
    0.00007030565f, 	0.0000274938f, 	0.0f,
    0.00006577532f, 	0.00002573083f, 	0.0f,
    0.00006151508f, 	0.00002407249f, 	0.0f,
    0.00005752025f, 	0.00002251704f, 	0.0f,
    0.00005378813f, 	0.0000210635f, 	0.0f,
    0.0000503135f, 	0.00001970991f, 	0.0f,
    0.00004708916f, 	0.00001845353f, 	0.0f,
    0.00004410322f, 	0.00001728979f, 	0.0f,
    0.0000413315f, 	0.00001620928f, 	0.0f,
    0.00003874992f, 	0.00001520262f, 	0.0f,
    0.00003633762f, 	0.00001426169f, 	0.0f,
    0.00003407653f, 	0.00001337946f, 	0.0f,
    0.00003195242f, 	0.00001255038f, 	0.0f,
    0.00002995808f, 	0.00001177169f, 	0.0f,
    0.00002808781f, 	0.00001104118f, 	0.0f,
    0.00002633581f, 	0.00001035662f, 	0.0f,
    0.0000246963f, 	0.000009715798f, 	0.0f,
    0.00002316311f, 	0.000009116316f, 	0.0f,
    0.00002172855f, 	0.000008555201f, 	0.0f,
    0.00002038519f, 	0.000008029561f, 	0.0f,
    0.00001912625f, 	0.000007536768f, 	0.0f,
    0.00001794555f, 	0.000007074424f, 	0.0f,
    0.00001683776f, 	0.000006640464f, 	0.0f,
    0.00001579907f, 	0.000006233437f, 	0.0f,
    0.00001482604f, 	0.000005852035f, 	0.0f,
    0.00001391527f, 	0.000005494963f, 	0.0f,
    0.00001306345f, 	0.000005160948f, 	0.0f,
    0.0000122672f, 	0.000004848687f, 	0.0f,
    0.00001152279f, 	0.000004556705f, 	0.0f,
    0.00001082663f, 	0.00000428358f, 	0.0f,
    0.0000101754f, 	0.000004027993f, 	0.0f,
    0.000009565993f, 	0.000003788729f, 	0.0f,
    0.000008995405f, 	0.000003564599f, 	0.0f,
    0.000008460253f, 	0.000003354285f, 	0.0f,
    0.000007957382f, 	0.000003156557f, 	0.0f,
    0.000007483997f, 	0.000002970326f, 	0.0f,
    0.000007037621f, 	0.000002794625f, 	0.0f,
    0.000006616311f, 	0.000002628701f, 	0.0f,
    0.000006219265f, 	0.000002472248f, 	0.0f,
    0.000005845844f, 	0.00000232503f, 	0.0f,
    0.000005495311f, 	0.000002186768f, 	0.0f,
    0.000005166853f, 	0.000002057152f, 	0.0f,
    0.000004859511f, 	0.000001935813f, 	0.0f,
    0.000004571973f, 	0.000001822239f, 	0.0f,
    0.00000430292f, 	0.000001715914f, 	0.0f,
    0.000004051121f, 	0.000001616355f, 	0.0f,
    0.000003815429f, 	0.000001523114f, 	0.0f,
    0.000003594719f, 	0.00000143575f, 	0.0f,
    0.000003387736f, 	0.000001353771f, 	0.0f,
    0.000003193301f, 	0.000001276714f, 	0.0f,
    0.000003010363f, 	0.000001204166f, 	0.0f,
    0.00000283798f, 	0.000001135758f, 	0.0f,
    0.000002675365f, 	0.000001071181f, 	0.0f,
    0.00000252202f, 	0.000001010243f, 	0.0f,
    0.000002377511f, 	0.0000009527779f, 	0.0f,
    0.000002241417f, 	0.0000008986224f, 	0.0f,
    0.000002113325f, 	0.0000008476168f, 	0.0f,
    0.00000199283f, 	0.0000007996052f, 	0.0f,
    0.000001879542f, 	0.0000007544361f, 	0.0f,
    0.000001773083f, 	0.0000007119624f, 	0.0f,
    0.000001673086f, 	0.0000006720421f, 	0.0f,
    0.000001579199f, 	0.000000634538f, 	0.0f
);

vec3 WaveToXYZ(in float wave) {
	// Conversion From Wavelength To XYZ Using CIE1964 XYZ Table
	vec3 XYZ = vec3(0.0, 0.0, 0.0);
	if ((wave >= 390) && (wave <= 830)){
		// Finding The Appropriate Index Of The Table For Given Wavelength
		int index = int((floor(wave) - 390.0));
		vec3 t1 = vec3(CIEXYZ2006[3*index], CIEXYZ2006[3*index+1], CIEXYZ2006[3*index+2]);
		vec3 t2 = vec3(CIEXYZ2006[3*index+3], CIEXYZ2006[3*index+4], CIEXYZ2006[3*index+5]);
		// Interpolation To Make The Colors Smooth
		XYZ = mix(t1, t2, wave - floor(wave));
	}
	return XYZ;
}

// http://www.songho.ca/opengl/gl_anglestoaxes.html
mat3 RotationMatrix(in vec3 angle) {
	// Builds Rotation Matrix Depending On Given Angle
	angle *= 0.0174532925199;
	vec3 sinxyz = sin(angle);
	vec3 cosxyz = cos(angle);
	mat3 mX = mat3(1.0, 0.0, 0.0, 0.0, cosxyz.x, -sinxyz.x, 0.0, sinxyz.x, cosxyz.x);
	mat3 mY = mat3(cosxyz.y, 0.0, sinxyz.y, 0.0, 1.0, 0.0, -sinxyz.y, 0.0, cosxyz.y);
	mat3 mZ = mat3(cosxyz.z, -sinxyz.z, 0.0, sinxyz.z, cosxyz.z, 0.0, 0.0, 0.0, 1.0);
	return mX * mY * mZ;
}

material GetMaterial(in int index) {
	// Extract Material Data From The Materials List
	material material;
	material.reflection.x = materials[5*index];
	material.reflection.y = materials[5*index+1];
	material.reflection.z = materials[5*index+2];
	material.emission.x = materials[5*index+3];
	material.emission.y = materials[5*index+4];
	return material;
}

bool SphereIntersection(in Ray ray, in sphere object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Sphere
	// Built By Solving The Equation: x^2 + y^2 + z^2 = r^2
	vec3 localorigin = ray.origin - object.pos;
	float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(ray.dir, localorigin);
	float c = dot(localorigin, localorigin) - (object.radius * object.radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	int isOutside = 1;
	if (discriminant < 0.0) {
		return false;
	}
	float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
	float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
	t = (t1 > 0.0) ? t1 : t2;
	isOutside = (t1 > 0.0) ? 1 : -1;
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = normalize(fma(ray.dir, vec3(t), localorigin) * isOutside);
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

bool PlaneIntersection(in Ray ray, in plane object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Plane
	// Built By Solving The Equation: z = 0
	vec3 localorigin = ray.origin - object.pos;
	float t = -localorigin.y / ray.dir.y;
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = faceforward(vec3(0.0, 1.0, 0.0), ray.dir, vec3(0.0, 1.0, 0.0));
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

// Box Intersection By iq
// https://www.shadertoy.com/view/ld23DV
bool BoxIntersection(in Ray ray, in box object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Box
	mat3 matrix = RotationMatrix(object.rotation);
	vec3 localorigin = (ray.origin - object.pos) * matrix;
	ray.dir = ray.dir * matrix;
	vec3 m = 1.0 / ray.dir;
	vec3 n = m * localorigin;
	vec3 k = abs(m) * object.size / 2.0;
	vec3 k1 = -n - k;
	vec3 k2 = -n + k;
	float tN = max(k1.x, max(k1.y, k1.z));
	float tF = min(k2.x, min(k2.y, k2.z));
	float t1 = min(tN, tF);
	float t2 = max(tN, tF);
	float t = 1e6;
	t = (t1 < 0.0) ? t2 : t1;
	if ((tN > tF) || (t < 1e-4)) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = (tN > 0.0) ? step(vec3(tN), k1) : step(k2, vec3(tF));
		normal *= -sign(ray.dir);
		normal = matrix * normalize(normal);
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

bool SphereSliceIntersection(in Ray ray, in sphereSlice object, in float localSlicePos, in bool isSideInvert, inout float hitdist, inout vec3 normal, inout int isOutside, inout material mat) {
	// Ray-Intersection Of Sliced Sphere
	// Slicing Based On Parameters Slice Size And Slice Side
	// Slicing Is Done Using 1D Interval Checks
	// Built By Solving The Same Equation Of Sphere
	mat3 matrix = RotationMatrix(object.rotation);
	Ray localRay;
	float sliceOffset = object.radius - object.sliceSize;
	localRay.origin = ray.origin - object.pos;
	localRay.origin = localRay.origin * matrix;
	if (object.is1stSlice) {
		localRay.origin.x += localSlicePos - object.sliceSize - sliceOffset;
	} else {
		localRay.origin.x -= localSlicePos - object.sliceSize - sliceOffset;
	}
	localRay.dir = ray.dir * matrix;
	float a = dot(localRay.dir, localRay.dir);
	float b = 2.0 * dot(localRay.dir, localRay.origin);
	float c = dot(localRay.origin, localRay.origin) - (object.radius * object.radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	int isOut = 1;
	if (discriminant < 0.0) {
		return false;
	}
	float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
	float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
	if (object.is1stSlice) {
		t1 = (fma(localRay.dir.x, t1, localRay.origin.x) > -sliceOffset) ? 1e6 : t1;
		t2 = (fma(localRay.dir.x, t2, localRay.origin.x) > -sliceOffset) ? 1e6 : t2;
	} else {
		t1 = (fma(localRay.dir.x, t1, localRay.origin.x) < sliceOffset) ? 1e6 : t1;
		t2 = (fma(localRay.dir.x, t2, localRay.origin.x) < sliceOffset) ? 1e6 : t2;
	}
	t = (t1 > 0.0) ? t1 : t;
	if (t2 < t) {
		t = t2;
		isOut = -1;
	}
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = matrix * normalize(fma(localRay.dir, vec3(t), localRay.origin) * isOut);
		isOutside = (!isSideInvert) ? isOut : -isOut;
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

void LensIntersection(in Ray ray, in lens object, inout float hitdist, inout vec3 normal, inout int isOutside, inout material mat) {
	// Ray-Intersection Of Lens
	// Done By Joining Two Slices Of Sphere
	// Thickness Of Lens Is Calculated By Using The Equation: thickness = 2(2f - sqrt(4f^2 - R^2))
	float lensThickness = 4.0 * object.focalLength - 2.0 * sqrt(4.0 * object.focalLength * object.focalLength - object.radius * object.radius);
	bool lensSlicePart[2] = {true, false};
	float lensSlicePos = 0.5 * (object.isConverging ? object.thickness : -object.thickness); // Gap Between Slices Of Lens
	if (object.isConverging) {
		lensSlicePos += lensThickness / 2.0;
	}
	for (int i = 0; i < 2; i++) {
		sphereSlice slice;
		slice.pos = object.pos;
		slice.radius = 2.0 * object.focalLength;
		slice.sliceSize = lensThickness / 2.0;
		slice.is1stSlice = lensSlicePart[i];
		slice.rotation = object.rotation;
		slice.materialID = object.materialID;
		SphereSliceIntersection(ray, slice, lensSlicePos, !object.isConverging, hitdist, normal, isOutside, mat);
	}
}

struct complex{
    float real;
    float imaginary;
};

complex add(in complex z, in float c) {
    return complex(z.real + c, z.imaginary);
}

complex add(in complex z1, in complex z2) {
    return complex(z1.real + z2.real, z1.imaginary + z2.imaginary);
}

complex invert(in complex z) {
    return complex(-z.real, -z.imaginary);
}

complex multiply(in complex z, in float c) {
    return complex(z.real * c, z.imaginary * c);
}

complex multiply(in complex z1, in complex z2) {
    return complex(z1.real * z2.real - z1.imaginary * z2.imaginary, z1.real * z2.imaginary + z1.imaginary * z2.real);
}

complex divide(in complex z, in float c) {
    return complex(z.real / c, z.imaginary / c);
}

complex divide(in complex z1, in complex z2) {
    return complex((z1.real * z2.real + z1.imaginary * z2.imaginary) / (z2.real * z2.real + z2.imaginary * z2.imaginary), (z1.imaginary * z2.real - z1.real * z2.imaginary) / (z2.real * z2.real + z2.imaginary * z2.imaginary));
}

float modulus(in complex z) {
    return sqrt(z.real * z.real + z.imaginary * z.imaginary);
}

complex pow(in complex z, in float n) {
	// Power Of Complex Number Calculator, Using De Moivre's Theorem

    // Convert Cartesian Form Into Polar Form
    float r = modulus(z);
	float theta = atan(z.imaginary, z.real);

    // De Moivre's Theorem
    complex new_z = complex(cos(n * theta) * pow(r, n), sin(n * theta) * pow(r, n));
    if ((new_z.real < 1e-6) && (new_z.real > -1e-6)) {
        new_z.real = 0.0;
    }
    if ((new_z.imaginary < 1e-6) && (new_z.imaginary > -1e-6)) {
        new_z.imaginary = 0.0;
    }

    return new_z;
}

void solveQuadratic(in float a, in complex b, in complex c, inout complex r1, inout complex r2) {
	// Solves The Quadratic Equation Having Complex Coefficients And Gives Out Complex Roots Using Quadratic Formula

    complex sqrtdiscriminant = pow(add(multiply(b, b), invert(multiply(c, 4.0 * a))), 0.5);
    r1 = divide(add(invert(sqrtdiscriminant), invert(b)), 2.0 * a);
    r2 = divide(add(sqrtdiscriminant, invert(b)), 2.0 * a);
}

void solveCubic(in float a, in float b, in float c, in float d, inout complex r1, inout complex r2, inout complex r3) {
	// Solves The Cubic Equation Having Real Coefficients And Gives Out Complex Roots Of The Cubic Equation Using General Cubic Formula

    float D0 = b * b - 3.0 * a * c;
    float D1 = 2.0 * b * b * b - 9.0 * a * b * c + 27.0 * a * a * d;
    complex C = pow(divide(add(pow(complex(D1 * D1 - 4.0 * D0 * D0 * D0, 0.0), 0.5), D1), 2.0), 1.0 / 3.0);

    complex e1 = complex(-0.5, 0.5 * sqrt(3.0));
    complex e2 = complex(-0.5, -0.5 * sqrt(3.0));
    complex e1C = multiply(e1, C);
    complex e2C = multiply(e2, C);
    r1 = invert(divide(add(add(divide(complex(D0, 0.0), C), C), b), 3.0 * a));
    r2 = invert(divide(add(add(divide(complex(D0, 0.0), e1C), e1C), b), 3.0 * a));
    r3 = invert(divide(add(add(divide(complex(D0, 0.0), e2C), e2C), b), 3.0 * a));

	// Set Imaginary To Zero If It Was Within Small Range Because Of Floating Point Precision Limit
    if ((r1.imaginary < 1e-6) && (r1.imaginary > -1e-6)) {
        r1.imaginary = 0.0;
    }
    if ((r2.imaginary < 1e-6) && (r2.imaginary > -1e-6)) {
        r2.imaginary = 0.0;
    }
    if ((r3.imaginary < 1e-6) && (r3.imaginary > -1e-6)) {
        r3.imaginary = 0.0;
    }
}

void solveQuartic(in float a4, in float a3, in float a2, in float a1, in float a0, inout complex a, inout complex b, inout complex c, inout complex d) {
	// Solves The Quartic Equation Having Real Coefficients And Gives Out Complex Roots Of The Quartic Equation Using An Algorithm
	// The Idea Of This Algorithm Can Be Found Here: https://people.math.harvard.edu/~landesman/assets/solving-the-cubic-and-quartic.pdf

	// Change The Quartic Equation Such That The Coefficient Of x^4 Is 1.
	float alpha = a3 / a4;
	float beta = a2 / a4;
	float gamma = a1 / a4;
	float delta = a0 / a4;

	// Solve The Cubic Equation
    complex r1 = complex(0.0, 0.0);
    complex r2 = complex(0.0, 0.0);
    complex r3 = complex(0.0, 0.0);
    solveCubic(1.0, -beta, alpha * gamma - 4.0 * delta, -(alpha * alpha * delta - 4.0 * beta * delta + gamma * gamma), r1, r2, r3);

	// Solve 3 Quadratics And Then Find The Roots Of Quartic Equation
    complex r11 = complex(0.0, 0.0);
    complex r12 = complex(0.0, 0.0);
    complex r21 = complex(0.0, 0.0);
    complex r22 = complex(0.0, 0.0);
    complex r31 = complex(0.0, 0.0);
    complex r32 = complex(0.0, 0.0);
    solveQuadratic(1.0, invert(r1), complex(delta, 0.0), r31, r32);
    solveQuadratic(1.0, invert(r2), complex(delta, 0.0), r11, r12);
    solveQuadratic(1.0, invert(r3), complex(delta, 0.0), r21, r22);
    a = divide(multiply(add(r21, r31), -gamma), add(multiply(add(r32, r22), add(add(r21, r31), r12)), multiply(r12, add(r21, r31))));
    b = divide(r11, a);
    c = divide(r21, a);
    d = divide(r31, a);
}

bool thritorius(in Ray ray, inout float hitdist, inout vec3 normal, inout material mat) {
	// I Will Add Other Comments Later, Currently I Don't Have Time For This
	vec3 o = ray.origin.xzy;
	vec3 d = ray.dir.xzy;
	float a4 = dot(vec3(d.x, d.y, 10.0 * d.z), d * d * d) + 2.0 * dot(d.xyz * d.xyz, d.yzx * d.yzx);
	float a3 = 4.0 * (dot(vec3(o.x, o.y, 10.0 * o.z), d * d * d) + dot(o * d, vec3(dot(d.yz, d.yz), dot(d.xz, d.xz), dot(d.xy, d.xy)))) + 2.0 * d.y * d.y * d.y - 6.0 * d.x * d.x * d.y;
	float a2 = 6.0 * dot(vec3(o.x, o.y, 10.0 * o.z), o * d * d) + 2.0 * (dot(o.xy * o.xy, d.yx * d.yx) + dot(o.xz * o.xz, d.zx * d.zx) + dot(o.yz * o.yz, d.zy * d.zy)) + 8.0 * dot(o.xxy * d.xxy, o.yzz * d.yzz) + 6.0 * o.y * d.y * d.y - 6.0 * o.y * d.x * d.x - 12.0 * o.x * d.x * d.y - 12.0 * d.z * d.z;
	float a1 = 4.0 * (dot(vec3(o.x, o.y, 10.0 * o.z) * o * o, d) + dot(o.xx * o.xx, o.yz * d.yz) + dot(o.yy * o.yy, o.xz * d.xz) + dot(o.zz * o.zz, o.xy * d.xy)) + 6.0 * o.y * o.y * d.y - 6.0 * o.x * o.x * d.y - 12.0 * o.x * o.y * d.x - 24.0 * o.z * d.z;
	float a0 = dot(vec3(o.x, o.y, 10.0 * o.z), o * o * o) + 2.0 * dot(o.xxy * o.xxy, o.yzz * o.yzz) + 2.0 * o.y * o.y * o.y - 6.0 * o.x * o.x * o.y - 12.0 * o.z * o.z + 1.0;

	complex r1 = complex(0.0, 0.0);
	complex r2 = complex(0.0, 0.0);
	complex r3 = complex(0.0, 0.0);
	complex r4 = complex(0.0, 0.0);
	solveQuartic(a4, a3, a2, a1, a0, r1, r2, r3, r4);

	float t = 1e6;
	if ((r1.imaginary < 1e-6) && (r1.imaginary > -1e-6)) {
		if ((r1.real < t) && (r1.real > 0.0)) {
			t = r1.real;
		}
	}
	if ((r2.imaginary < 1e-6) && (r2.imaginary > -1e-6)) {
		if ((r2.real < t) && (r2.real > 0.0)) {
			t = r2.real;
		}
	}
	if ((r3.imaginary < 1e-6) && (r3.imaginary > -1e-6)) {
		if ((r3.real < t) && (r3.real > 0.0)) {
			t = r3.real;
		}
	}
	if ((r4.imaginary < 1e-6) && (r4.imaginary > -1e-6)) {
		if ((r4.real < t) && (r4.real > 0.0)) {
			t = r4.real;
		}
	}

	if (t < hitdist) {
		hitdist = t;
		float x = o.x + d.x * t;
		float y = o.y + d.y * t;
		float z = o.z + d.z * t;
		normal.x = 4.0 * x * x * x + 4.0 * x * y * y - 12.0 * x * y + 4.0 * x * z * z;
		normal.y = 40.0 * z * z * z + 4.0 * x * x * z + 4.0 * y * y * z - 24.0 * z;
		normal.z = 4.0 * y * y * y + 4.0 * x * x * y - 6.0 * x * x + 6.0 * y * y + 4.0 * y * z * z;
		normal = normalize(normal);
		mat = GetMaterial(1);
		return true;
	}
	return false;
}

float Intersection(in Ray ray, inout vec3 normal, inout material mat) {
	// Finds The Ray-Intersection Of Every Object In The Scene
	float hitdist = 1e6;
	int offset = 0;
	// Iterate Over All The Spheres In The Scene
	for (int i = 0; i < numObjects[0]; i++) {
		sphere object;
		object.pos = vec3(objects[5*i+offset], objects[5*i+1+offset], objects[5*i+2+offset]);
		object.radius = objects[5*i+3+offset];
		object.materialID = int(objects[5*i+4+offset])-1;
		SphereIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 5*numObjects[0];
	// Iterate Over All The Planes In The Scene
	for (int i = 0; i < numObjects[1]; i++) {
		plane object;
		object.pos = vec3(objects[4*i+offset], objects[4*i+1+offset], objects[4*i+2+offset]);
		object.materialID = int(objects[4*i+3+offset])-1;
		PlaneIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 4*numObjects[1];
	// Iterate Over All The Cubes In The Scene
	for (int i = 0; i < numObjects[2]; i++) {
		box object;
		object.pos = vec3(objects[10*i+offset], objects[10*i+1+offset], objects[10*i+2+offset]);
		object.rotation = vec3(objects[10*i+3+offset], objects[10*i+4+offset], objects[10*i+5+offset]);
		object.size = vec3(objects[10*i+6+offset], objects[10*i+7+offset], objects[10*i+8+offset]);
		object.materialID = int(objects[10*i+9+offset])-1;
		BoxIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 10*numObjects[2];
	// Iterate Over All The Lenses In The Scene
	for (int i = 0; i < numObjects[3]; i++) {
		lens object;
		object.pos = vec3(objects[11*i+offset], objects[11*i+1+offset], objects[11*i+2+offset]);
		object.rotation = vec3(objects[11*i+3+offset], objects[11*i+4+offset], objects[11*i+5+offset]);
		object.radius = objects[11*i+6+offset];
		object.focalLength = objects[11*i+7+offset];
		object.thickness = objects[11*i+8+offset];
		object.isConverging = bool(objects[11*i+9+offset]);
		object.materialID = int(objects[11*i+10+offset])-1;
		int isOutside = 1;
		LensIntersection(ray, object, hitdist, normal, isOutside, mat);
	}
	offset += 11*numObjects[3];
	// Temporarily Removed Thritorius Due To Bugs
	//ray.origin -= vec3(1.0, 1.0, -7.0);
	//thritorius(ray, hitdist, normal, mat);

	return hitdist;
}

// https://www.pcg-random.org/
void PCG32(inout uint seed) {
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	seed = (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed) {
	PCG32(seed);
	return float(seed) / 0xFFFFFFFFu;
}

uint GenerateSeed(in uvec2 xy, in uint k) {
	// Actually This Is Not The Correct Way To Generate Seed
	// This Is The Correct Implementation Which Has No Overlapping:
	/* uint seed = (resolution.x * resolution.y) * (frame - 1) + k;
	   seed += xy.x + resolution.x * xy.y;*/
	// But Because This Seed Crosses 32-Bit Limit Quickly, And Implementing In 64-Bit Makes Path Tracer Much Slower,
	// That's Why I Implemented This Trick. Even If Pixels Seed Overlap With Other Pixels Somewhere, It Won't Affect The Result
	uint seed = (frame - 1) + k;
	PCG32(seed);
	seed = uint(mod(double(seed) + double(xy.x + resolution.x * xy.y), 0xFFFFFFFFu));
	return seed;
}

float SampleSpectra(in float start, in float end, in float rand){
	// Uniform Inverted CDF For Sampling
	return (end - start) * rand + start;
}

float InvSpectraPDF(in float start, in float end){
	// Inverse Of Uniform PDF
	return end - start;
}

vec2 SampleUniformUnitDisk(inout uint seed) {
	// Samples Uniformly Distributed Random Points On Unit Disk
	float phi = 2.0 * PI * RandomFloat(seed);
	float d = sqrt(RandomFloat(seed));
	return d * vec2(cos(phi), sin(phi));
}

vec3 SampleUniformUnitSphere(inout uint seed){
	// Samples Uniformly Distributed Random Points On Unit Sphere
	// XYZ Coordinates Is Calculated Based On Longitude And Latitude
	// Longitude Is Generated Uniformly And Sin Of Latitude Is Generated Uniformly
	// Reason: If We Generate Latitude Uniformly, The Top And Bottom Of The Sphere Will Have More Points Than Other Regions
	float phi = 2.0 * PI * RandomFloat(seed);
	float sintheta = 2.0 * RandomFloat(seed) - 1.0;
	float costheta = sqrt(1.0 - sintheta * sintheta);
	float x = cos(phi) * costheta;
	float y = sin(phi) * costheta;
	float z = sintheta;
	return vec3(x, y, z);
}

vec3 SampleCosineDirectionHemisphere(in vec3 normal, inout uint seed){
	// Generate Cosine Distributed Random Vectors Within Normals Hemisphere
	vec3 sumvector = normal + SampleUniformUnitSphere(seed);
	return normalize(sumvector);
}

float CosineDirectionPDF(in float costheta) {
	// Idea: Integrating Over Cosine PDF Must Give 1
    // Solution: Divide Cosine By Pi
    // Note: Integrating Equation Is Solid Angle. Must Insert PDF Inside Integrals To Normalize
	return costheta / PI;
}

float SpectralPowerDistribution(in float l, in float l_peak, in float d, in int invert) {
	// Spectral Power Distribution Function Calculated On The Basis Of Peak Wavelength And Standard Deviation
	// Using Gaussian Function To Predict Spectral Radiance
	// In Reality, Spectral Radiance Function Has Different Shapes For Different Objects Also Looks Much Different Than This
	float x = (l - l_peak) / (2.0 * d * d);
	float radiance = exp(-x * x);
	radiance = mix(radiance, 1.0 - radiance, invert);
	return radiance;
}

float BlackBodyRadiation(in float l, in float T) {
	// Plank's Law
	return (1.1910429724e-16 * pow(l, -5.0)) / (exp(0.014387768775 / (l * T)) - 1.0);
}

float BlackBodyRadiationPeak(in float T) {
	// Derived By Substituting Wien's Displacement Law On Plank's Law
	return 4.0956746759e-6 * pow(T, 5.0);
}

float Emit(in float l, in material mat) {
	// Calculates Light Emittance Based On Given Material
	float temperature = max(mat.emission.x, 0.0);
	float lightEmission = (BlackBodyRadiation(l * 1e-9, temperature) / BlackBodyRadiationPeak(temperature)) * max(mat.emission.y, 0.0);
	return lightEmission;
}

float RefractiveIndexWavelength(in float l, in float n, in float l_n, in float s){
	// My Own Function For Refractive Index
	// Function Is Based On Observation How Graph Of Mathematrical Functions Look Like
	// Made To Produce Change In Refractive Index Based On Wavelength
	return fma(s, (l_n / l) - 1.0, n);
}

float RefractiveIndexBK7Glass(in float l) {
	// Sellmeier Equation For Refractive Index Of BK7 Glass
	l *= 1e-3;
	float l2 = l * l;
	float n2 = 1.0;
	n2 += (1.03961212 * l2) / (l2 - 6.00069867e-3);
	n2 += (0.231792344 * l2) / (l2 - 2.00179144e-2);
	n2 += (1.01046945 * l2) / (l2 - 1.03560653e2);
	return sqrt(n2);
}

float EvaluateBRDF(in float l, in vec3 inDir, in vec3 outDir, in vec3 normal, in material mat) {
	// Evaluate The BRDF
	// Lambertian BRDF For Diffuse Surface
	float diffuse = SpectralPowerDistribution(l, mat.reflection.x, mat.reflection.y, int(mat.reflection.z)) / PI;
	return diffuse;
}

vec3 SampleBRDF(in vec3 inDir, in vec3 normal, inout uint seed) {
	// Sample Directions Of BRDF
	vec3 outDir = SampleCosineDirectionHemisphere(normal, seed);
	return outDir;
}

float BRDFPDF(in vec3 outDir, in vec3 normal) {
	// PDF For Sampling Directions Of BRDF
	return CosineDirectionPDF(dot(outDir, normal));
}

float TraceRay(in float l, inout float rayradiance, inout Ray inRay, inout uint seed, out bool isTerminate) {
	// Traces A Ray Along The Given Origin And Direction Then Calculates Light Interactions
	float radiance = 0.0;
	vec3 normal = vec3(0.0);
	material mat;
	float hitdist = Intersection(inRay, normal, mat);
	Ray outRay = inRay;
	if (hitdist < MAXDIST) {
		// Calculate The Next Ray's Origin And Direction
		outRay.origin = fma(inRay.dir, vec3(hitdist), inRay.origin);
		outRay.dir = SampleBRDF(inRay.dir, normal, seed);
		float BRDFpdf = BRDFPDF(outRay.dir, normal);
		// If The Ray Hits The Light Source
		if (mat.emission.y > 0.0) {
			radiance = fma(Emit(l, mat), rayradiance, radiance);
			// Terminate The Path If The Ray Hits The Light Source
			isTerminate = true;
			return radiance;
		}
		// Evaluate The BRDF
		float costheta = dot(outRay.dir, normal);
		rayradiance *= EvaluateBRDF(l, inRay.dir, outRay.dir, normal, mat) * costheta / BRDFpdf;
		// Russian Roulette
		// Probability Of The Ray Can Be Anything From 0 To 1
		float rayProbability = clamp(rayradiance, 0.0, 0.99);
		if (RandomFloat(seed) > rayProbability) {
			// Randomly Terminate Ray Based On Probability
			isTerminate = true;
			return radiance;
		}
		// Add Energy Which Was Lost By Terminating Rays
		rayradiance *= 1.0 / rayProbability;
		inRay = outRay;
	}
	else {
		isTerminate = true;
	}
	return radiance;
}

float TracePath(in float l, in Ray ray, inout uint seed) {
	// Traces A Path Starting From The Given Origin And Direction
	// And Calculates Light Radiance
	float radiance = 0.0;
	float rayradiance = 1.0;
	bool isTerminate = false;
	for (int i = 0; i < pathLength; i++) {
		radiance += TraceRay(l, rayradiance, ray, seed, isTerminate);
		if (isTerminate){
			break;
		}
	}
	return radiance;
}

void TracePathLens(in float l, inout Ray ray, in vec3 forwardDir) {
	// Trace The Path Through The BiConvex Lens
	lens object;
	object.radius = lensData.x;
	object.focalLength = lensData.y;
	object.thickness = lensData.z;
	object.isConverging = true;
	object.pos = cameraPos + forwardDir * lensData.w;
	object.rotation = vec3(0.0, 90.0 - cameraAngle.y, cameraAngle.x);
	object.materialID = 0;
	for (int i = 0; i < 2; i++) {
		float hitdist = 1e6;
		vec3 normal = vec3(0.0);
		int isOutside = 1;
		material mat;
		LensIntersection(ray, object, hitdist, normal, isOutside, mat);
		// Calculate The Refractive Index
		float n1 = 1.0;
		float n2 = 1.0;
		if (isOutside == 1) {
			n1 = 1.0;
			n2 = RefractiveIndexBK7Glass(l);
		} else {
			n1 = RefractiveIndexBK7Glass(l);
			n2 = 1.0;
		}
		float n12 = n1 / n2;
		// Wavelength Changes As Refractive Index Changes
		// lambda = lamda_0 / n
		l = l * n12;
		// Calculate The Next Ray's Origin And Direction
		ray.origin = fma(ray.dir, vec3(hitdist), ray.origin);
		ray.dir = refract(ray.dir, normal, n12);
	}
}

vec3 Scene(in uvec2 xy, in vec2 uv, in uint k) {
	uint seed = GenerateSeed(xy, k);
	// SSAA
	uv += vec2(2.0 * RandomFloat(seed) - 0.5, 2.0 * RandomFloat(seed) - 0.5) / resolution;

	// This Is A Simple Camera Made Up Of A BiConvex Lens And An Aperture
	// Ray Originates From The Pixel Of Camera Sensor
	// Then Passes Through The Area Of Aperture
	// Then It Passes Through A BiConvex Lens
	Ray ray;
	mat3 matrix = RotationMatrix(vec3(cameraAngle, 0.0));
	uv *= -cameraSize * 0.5;
	// Position Of Each Pixel On Sensor As Ray Origin
	ray.origin = cameraPos + (vec3(uv, 0.0) * matrix);
	// Generates Random Point On Aperture
	vec3 pointOnAperture = cameraPos + (vec3(0.5 * apertureSize * SampleUniformUnitDisk(seed), apertureDist) * matrix);
	// Compute Random Direction Which Passes Through The Area Of Aperture From Camera Sensor
	ray.dir = normalize(pointOnAperture - ray.origin);

	vec3 color = vec3(0.0);

	float l = SampleSpectra(390.0, 720.0, RandomFloat(seed));
	vec3 forwardDir = vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
	// Ray Passes Through The Lens Here
	TracePathLens(l, ray, forwardDir);
	color += TracePath(l, ray, seed) * WaveToXYZ(l) * InvSpectraPDF(390.0, 720.0);
	
	// Debug
	//vec3 normal = vec3(0.0);
	//material mat;
	//color += Intersection(ray, normal, mat);

	return color;
}

void Accumulate(inout vec3 color) {
	// Temporal Accumulation Based On Given Parameters When Scene Is Dynamic And Accumulation When Scene Is Static
	// Simulation Of Persistance Using Temporal Accumulation
	// The Idea Is To Multiply Color Value By 1/256 In A Number Of Frames
	// Find The Constant Based On Given Parameters
	// Then We Get, x^numFrames = 2^(-8) Where x Is Multiply Constant
	// Also We Know That, numFrames = FPS * time(The Amount Of Time Will Be Needed To Reach 1/256)
	// Therefore, x = 2^(-8 / (FPS*time))
	if (prevSamples > 0) {
		if ((samples == samplesPerFrame) && (frame > samplesPerFrame)) {
			float weight = pow(2.0, -8.0 / (FPS * persistance));
			color = ((1.0 - weight) * color) + (weight * texture(screenTexture, gl_FragCoord.xy / resolution).xyz * samplesPerFrame / prevSamples);
		} else {
			color = texture(screenTexture, gl_FragCoord.xy / resolution).xyz + color;
		}
	}
}

void main() {
	uvec2 xy = uvec2(gl_FragCoord.xy);
	vec2 uv = ((2.0 * vec2(xy) - resolution) / resolution.y);

	vec3 color = vec3(0.0);
	for (uint i = 0; i < samplesPerFrame; i++) {
		color += Scene(xy, uv, i);
	}
	// Simulate Exposure Variance Depending On Aperture Size And ISO
	color *= apertureSize * apertureSize * ISO;
	Accumulate(color);

	Fragcolor = vec4(color, 1.0);
}